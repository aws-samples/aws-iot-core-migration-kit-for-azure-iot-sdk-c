// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

/* This sample shows how to translate the Device Twin json received from Azure IoT Hub into meaningful data for your application.
 * It uses the parson library, a very lightweight json parser.
 *
 * There is an analogous sample using the serializer - which is a library provided by this SDK to help parse json - in devicetwin_simplesample.
 * Most applications should use this sample, not the serializer.
 *
 * WARNING: Check the return of all API calls when developing your solution. Return checks ommited for sample simplification. */

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include "azure_macro_utils/macro_utils.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/platform.h"
#include "iothub_device_client.h"
#include "iothub_client_options.h"
#include "iothub.h"
#include "iothub_message.h"
#include "parson.h"
#include "azure_c_shared_utility/shared_util_options.h"

#include "azure_umqtt_c/mqtt_client.h"

#include "iothubtransportmqtt.h"

#include "certs.h"

static const char * connectionString = "HostName=abcd1234567890-ats.iot.us-east-2.amazonaws.com;DeviceId=mything;x509=true";

static const char * x509certificate = NULL;

static const char * x509privatekey = NULL;

typedef struct MAKER_TAG
{
    char * makerName;
    char * style;
    int year;
} Maker;

typedef struct GEO_TAG
{
    double longitude;
    double latitude;
} Geo;

typedef struct CAR_STATE_TAG
{
    int32_t softwareVersion;        /* reported property */
    uint8_t reported_maxSpeed;      /* reported property */
    char * vanityPlate;             /* reported property */
} CarState;

typedef struct CAR_SETTINGS_TAG
{
    uint8_t desired_maxSpeed;       /* desired property */
    Geo location;                   /* desired property */
} CarSettings;

typedef struct CAR_TAG
{
    char * lastOilChangeDate;       /* reported property */
    char * changeOilReminder;       /* desired property */
    Maker maker;                    /* reported property */
    CarState state;                 /* reported property */
    CarSettings settings;           /* desired property */
} Car;

/* Converts the Car object into a JSON blob with reported properties that is ready to be sent across the wire as a twin. */
static char * serializeToJson( Car * car )
{
    char * result;

    JSON_Value * root_value = json_value_init_object();
    JSON_Object * root_object = json_value_get_object( root_value );

    /* Only reported properties: */
    ( void ) json_object_set_string( root_object, "lastOilChangeDate", car->lastOilChangeDate );
    ( void ) json_object_dotset_string( root_object, "maker.makerName", car->maker.makerName );
    ( void ) json_object_dotset_string( root_object, "maker.style", car->maker.style );
    ( void ) json_object_dotset_number( root_object, "maker.year", car->maker.year );
    ( void ) json_object_dotset_number( root_object, "state.reported_maxSpeed", car->state.reported_maxSpeed );
    ( void ) json_object_dotset_number( root_object, "state.softwareVersion", car->state.softwareVersion );
    ( void ) json_object_dotset_string( root_object, "state.vanityPlate", car->state.vanityPlate );

    result = json_serialize_to_string( root_value );

    json_value_free( root_value );

    return result;
}

/* Converts the desired properties of the Device Twin JSON blob received from IoT Hub into a Car object. */
static Car * parseFromJson( const char * json, DEVICE_TWIN_UPDATE_STATE update_state )
{
    Car * car = malloc( sizeof( Car ) );
    JSON_Value * root_value = NULL;
    JSON_Object * root_object = NULL;

    if ( NULL == car )
    {
        printf( "ERROR: Failed to allocate memory\r\n" );
    }
    else
    {
        ( void ) memset( car, 0, sizeof( Car ) );

        root_value = json_parse_string( json );
        root_object = json_value_get_object( root_value );

        /* Only desired properties: */
        JSON_Value * changeOilReminder;
        JSON_Value * desired_maxSpeed;
        JSON_Value * latitude;
        JSON_Value * longitude;

        if ( update_state == DEVICE_TWIN_UPDATE_COMPLETE )
        {
            changeOilReminder = json_object_dotget_value( root_object, "desired.changeOilReminder" );
            desired_maxSpeed = json_object_dotget_value( root_object, "desired.settings.desired_maxSpeed" );
            latitude = json_object_dotget_value( root_object, "desired.settings.location.latitude" );
            longitude = json_object_dotget_value( root_object, "desired.settings.location.longitude" );
        }
        else
        {
            changeOilReminder = json_object_dotget_value( root_object, "changeOilReminder" );
            desired_maxSpeed = json_object_dotget_value( root_object, "settings.desired_maxSpeed" );
            latitude = json_object_dotget_value( root_object, "settings.location.latitude" );
            longitude = json_object_dotget_value( root_object, "settings.location.longitude" );
        }

        if ( changeOilReminder != NULL )
        {
            const char * data = json_value_get_string( changeOilReminder );

            if ( data != NULL )
            {
                car->changeOilReminder = malloc( strlen( data ) + 1 );
                if ( NULL != car->changeOilReminder )
                {
                    ( void ) strcpy( car->changeOilReminder, data );
                }
            }
        }

        if ( desired_maxSpeed != NULL )
        {
            car->settings.desired_maxSpeed = ( uint8_t ) json_value_get_number( desired_maxSpeed );
        }

        if ( latitude != NULL )
        {
            car->settings.location.latitude = json_value_get_number( latitude );
        }

        if ( longitude != NULL )
        {
            car->settings.location.longitude = json_value_get_number( longitude );
        }
        json_value_free( root_value );
    }

    return car;
}

static int deviceMethodCallback( const char * method_name,
                                 const unsigned char * payload,
                                 size_t size,
                                 unsigned char ** response,
                                 size_t * response_size, void * userContextCallback )
{
    ( void ) userContextCallback;
    ( void ) payload;
    ( void ) size;

    int result;

    if ( strcmp( "getCarVIN", method_name ) == 0 )
    {
        const char deviceMethodResponse[] = "{ \"Response\": \"1HGCM82633A004352\" }";
        *response_size = sizeof( deviceMethodResponse ) - 1;
        *response = malloc( *response_size );
        ( void ) memcpy( *response, deviceMethodResponse, *response_size );
        result = 200;
    }
    else
    {
        /* All other entries are ignored. */
        const char deviceMethodResponse[] = "{ }";
        *response_size = sizeof( deviceMethodResponse ) - 1;
        *response = malloc( *response_size );
        ( void ) memcpy( *response, deviceMethodResponse, *response_size );
        result = -1;
    }

    return result;
}

static void getCompleteDeviceTwinOnDemandCallback( DEVICE_TWIN_UPDATE_STATE update_state,
                                                   const unsigned char * payLoad,
                                                   size_t size,
                                                   void * userContextCallback )
{
    ( void ) update_state;
    ( void ) userContextCallback;
    printf( "GetTwinAsync result:\r\n%.*s\r\n", (int)size, payLoad );
}

static void deviceTwinCallback( DEVICE_TWIN_UPDATE_STATE update_state,
                                const unsigned char * payLoad,
                                size_t size,
                                void * userContextCallback )
{
    ( void ) update_state;
    ( void ) size;

    Car * oldCar = ( Car * ) userContextCallback;
    Car * newCar = parseFromJson( ( const char * ) payLoad, update_state );

    if ( newCar->changeOilReminder != NULL )
    {
        if ( ( oldCar->changeOilReminder != NULL ) && ( strcmp( oldCar->changeOilReminder, newCar->changeOilReminder ) != 0 ) )
        {
            free( oldCar->changeOilReminder );
        }

        if ( oldCar->changeOilReminder == NULL )
        {
            printf( "Received a new changeOilReminder = %s\n", newCar->changeOilReminder );
            if ( NULL != ( oldCar->changeOilReminder = malloc( strlen( newCar->changeOilReminder ) + 1 ) ) )
            {
                ( void ) strcpy( oldCar->changeOilReminder, newCar->changeOilReminder );
                free( newCar->changeOilReminder );
            }
        }
    }

    if ( newCar->settings.desired_maxSpeed != 0 )
    {
        if ( newCar->settings.desired_maxSpeed != oldCar->settings.desired_maxSpeed )
        {
            printf( "Received a new desired_maxSpeed = %" PRIu8 "\n", newCar->settings.desired_maxSpeed );
            oldCar->settings.desired_maxSpeed = newCar->settings.desired_maxSpeed;
        }
    }

    if ( newCar->settings.location.latitude != 0 )
    {
        if ( newCar->settings.location.latitude != oldCar->settings.location.latitude )
        {
            printf( "Received a new latitude = %f\n", newCar->settings.location.latitude );
            oldCar->settings.location.latitude = newCar->settings.location.latitude;
        }
    }

    if ( newCar->settings.location.longitude != 0 )
    {
        if ( newCar->settings.location.longitude != oldCar->settings.location.longitude )
        {
            printf( "Received a new longitude = %f\n", newCar->settings.location.longitude );
            oldCar->settings.location.longitude = newCar->settings.location.longitude;
        }
    }

    free( newCar );
}

static void reportedStateCallback( int status_code,
                                   void * userContextCallback )
{
    ( void ) userContextCallback;
    printf( "Device Twin reported properties update completed with result: %d\r\n", status_code );
}

static IOTHUBMESSAGE_DISPOSITION_RESULT messageCallback( IOTHUB_MESSAGE_HANDLE message,
                                                         void * user_context )
{
    const unsigned char * buff_msg;
    size_t buff_len;

    const char * topic = IoTHubMessage_GetProperty( message, "topic" );
    if ( topic != NULL )
    {
        printf( "topic: %s\r\n", topic );
    }

    if ( IoTHubMessage_GetByteArray( message, &buff_msg, &buff_len ) == IOTHUB_MESSAGE_OK )
    {
        /* buff_msg points to underlying buffer of message, and iothub client will free it properly. */
        printf( "Payload(%d): \r\n%.*s\r\n", ( int ) buff_len, ( int ) buff_len, buff_msg );
    }

    return IOTHUBMESSAGE_ACCEPTED;
}

int main( void ) 
{
    IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol;
    IOTHUB_DEVICE_CLIENT_LL_HANDLE device_ll_handle;

    protocol = MQTT_Protocol;

    IoTHub_Init();

    if ( ( device_ll_handle = IoTHubDeviceClient_LL_CreateFromConnectionString( connectionString, protocol ) ) == NULL )
    {
        printf( "ERROR: iotHubClientHandle is NULL!\r\n" );
    }
    else
    {
        bool traceOn = true;
        IoTHubDeviceClient_LL_SetOption( device_ll_handle, OPTION_LOG_TRACE, &traceOn );

#ifdef SET_TRUSTED_CERT_IN_SAMPLES
        /* Setting the Trusted Certificate.  This is only necessary on system with without
         * built in certificate stores. */
        IoTHubDeviceClient_LL_SetOption( device_ll_handle, OPTION_TRUSTED_CERT, certificates );
#endif /* SET_TRUSTED_CERT_IN_SAMPLES */

        IoTHubDeviceClient_LL_SetOption( device_ll_handle, OPTION_X509_CERT, x509certificate );
        IoTHubDeviceClient_LL_SetOption( device_ll_handle, OPTION_X509_PRIVATE_KEY, x509privatekey );

        Car car;
        memset( &car, 0, sizeof( Car ) );
        car.lastOilChangeDate = "2016";
        car.maker.makerName = "Fabrikam";
        car.maker.style = "sedan";
        car.maker.year = 2014;
        car.state.reported_maxSpeed = 100;
        car.state.softwareVersion = 1;
        car.state.vanityPlate = "1I1";

        char * reportedProperties = serializeToJson( &car );

        IoTHubClientCore_LL_SetDeviceMethodCallback( device_ll_handle, deviceMethodCallback, NULL );
        IoTHubClientCore_LL_SetDeviceTwinCallback( device_ll_handle, deviceTwinCallback, &car );
        IoTHubClientCore_LL_SetMessageCallback( device_ll_handle, messageCallback, NULL );

        IoTHubClientCore_LL_GetTwinAsync( device_ll_handle, getCompleteDeviceTwinOnDemandCallback, NULL );
        IoTHubClientCore_LL_SendReportedState( device_ll_handle, ( const unsigned char * ) reportedProperties, strlen( reportedProperties ), reportedStateCallback, NULL );
        for ( int i = 0; i < 2000; i++ )
        {
            IoTHubDeviceClient_LL_DoWork( device_ll_handle );
            ThreadAPI_Sleep( 1 );
        }

        IoTHubDeviceClient_LL_SetOption( device_ll_handle, "mqtt_subscribe", "mytopic" );

        MQTT_MESSAGE_HANDLE mqttMsg = mqttmessage_create( 0, "mytopic", DELIVER_AT_LEAST_ONCE, "Hello World!", sizeof( "Hello World" ) - 1 );
        IoTHubDeviceClient_LL_SetOption( device_ll_handle, "mqtt_publish", mqttMsg );
        mqttmessage_destroy( mqttMsg );
        for ( int i = 0; i < 2000; i++ )
        {
            IoTHubDeviceClient_LL_DoWork( device_ll_handle );
            ThreadAPI_Sleep( 1 );
        }

        IoTHubDeviceClient_LL_SetOption( device_ll_handle, "mqtt_unsubscribe", "mytopic" );
        IoTHubDeviceClient_LL_Destroy( device_ll_handle );
    }

    IoTHub_Deinit();
    return 0;
}