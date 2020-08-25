# Change to Azure IoT Device SDK for C for Interacting with AWS IoT Core

Azure IoT Device SDK for C is a set of libraries for interacting with Azure IoT Hub service.  Applications that use MQTT to interact with the cloud using the Azure SDK can interact with AWS IoT Core service as well. This document describes how to do so by applying a change to the Azure IoT Device SDK for C.  In many cases, the application code will not need a changes in order to achieve this.

# Introduction

This document is a step-by-step guide. It includes:

*   [Get and build Azure IoT device SDK for C](#Get-and-build-Azure-IoT-device-SDK-for-C)
*   [Setup Azure IoT Hub for x509 connection, and run sample code](#Setup-Azure-IoT-Hub-for-x509-connection,-and-run-sample-code)
    *   [Setup a device](#Setup-a-device)
    *   [Create and manage x.509 certificate](#Create-and-manage-x.509-certificate)
    *   [Run x.509 sample code](#Run-x509-sample-code)
*   [Setup AWS IoT Core to use the same x.509 certificate](#Setup-AWS-IoT-Core-to-use-the-same-x.509-certificate)
    *   [Register root CA](#Register-root-CA)
    *   [Create policy](#Create-policy)
    *   [Create thing](#Create-thing)
    *   [Verify the certificate](#Verify-the-certificate)
*   [Apply patch to Azure IoT device SDK for C, run sample code to connect to AWS IoT Core](#Apply-patch-to-Azure-IoT-device-SDK-for-C,-run-sample-code-to-connect-to-AWS-IoT-Core)
    *   [Run x.509 sample code with patch](#Run-x.509-sample-code-with-patch)
    *   [Run Device Twins example while it connects to AWS IoT Core Device Shadow](#Run-Device-Twins-example-while-it-connects-to-AWS-IoT-Core-Device-Shadow)
    *   [Verify Direct Methods on AWS IoT Core](#Verify-Direct-Methods-on-AWS-IoT-Core)
    *   [Verify Direct Methods over HTTP](#Verify-Direct-Methods-over-HTTP)
    *   [Go back to Azure IoT for debugging](#Go-back-to-Azure-IoT-for-debugging)
*   [Optional Features](#Optional-Features)
    * [Subscribe and publish custom topics](#Subscribe-and-publish-custom-topics)
*   [FAQ](#FAQ)
    *   [MQTT payload size limitation caused by MBEDTLS_SSL_MAX_FRAGMENT_LENGTH](#MQTT-payload-size-limitation-caused-by-MBEDTLS_SSL_MAX_FRAGMENT_LENGTH)

Azure IoT devicde SDK for C supports Linux, Mac OS, and Visual Studio. We have used Ubuntu 18.04 on WSL for writing the procedures in this document.

# Get and build Azure IoT device SDK for C

You can skip this section if you are already familiar with Azure IoT device SDK for C.

You can find the project of Azure IoT device SDK here: https://github.com/Azure/azure-iot-sdk-c

Install all dependencies described here: https://github.com/Azure/azure-iot-sdk-c/blob/master/doc/devbox_setup.md#linux

```
sudo apt-get update
sudo apt-get install -y git cmake build-essential curl libcurl4-openssl-dev libssl-dev uuid-dev
```

Get Azure IoT device SDK. There are several LTS (long-term support) branches on GitHub. We use the LTS-02-2020 branch here.

```
git clone -b lts_02_2020 git@github.com:Azure/azure-iot-sdk-c.git
cd azure-iot-sdk-c
git submodule update --init
```

Configure and build:

```
cd azure-iot-sdk-c
mkdir cmake
cmake ..
cmake --build . -- -j4
```

# Setup Azure IoT Hub for x509 connection, and run sample code

In this section, we will setup a device on Azure IoT Hub, then configure a x.509 certificate.

## Setup a device

Go to Azure IoT Hub home page: https://azure.microsoft.com/en-us/services/iot-hub/

Select Portal.

![](doc/images/azure_iot_hub_setup_a_device_01.png)

Select IoT Hub.

![](doc/images/azure_iot_hub_setup_a_device_02.png)

Select “Add” to create a new IoT Hub.

![](doc/images/azure_iot_hub_setup_a_device_03.png)

Configure “Resource group”, “Region” and “IoT hub name”. Then select “Review + create”

![](doc/images/azure_iot_hub_setup_a_device_04.png)

Review all settings, then select “Create”.

![](doc/images/azure_iot_hub_setup_a_device_05.png)

It would take a few minutes to finish.

Go back to IoT Hub portal. Select the IoT Hub that we just created.

![](doc/images/azure_iot_hub_setup_a_device_06.png)

Select “IoT devices”

![](doc/images/azure_iot_hub_setup_a_device_07.png)

Select “New” to create a new device.

![](doc/images/azure_iot_hub_setup_a_device_08.png)


Fill in “device ID“, and select ”X.509 CA Signed“ in Authentication type. Then select ”Save”

![](doc/images/azure_iot_hub_setup_a_device_09.png)

Now you should see a new device in you IoT Hub devices list.

![](doc/images/azure_iot_hub_setup_a_device_10.png)


## Create and Manage x.509 certificate

You can get detail information from this page: https://github.com/Azure/azure-iot-sdk-c/blob/master/tools/CACertificates/CACertificateOverview.md

Copy certificates tools from Azure IoT SDK to your certificates folder:

```
cp -rf azure-iot-sdk-c/tools/CACertificates/ ./mycerts
cd mycerts
chmod 700 certGen.sh
```

Create CA and intermediate certificate.

```
./certGen.sh `create_root_and_intermediate`
```

Go to IoT Hub portal, select “Certificates” in you IoT Hub.

![](doc/images/azure_iot_hub_setup_x509_01.png)

Select “Add”

![](doc/images/azure_iot_hub_setup_x509_02.png)

Pick a "Certificate Name". Upload the root CA files. It should be this file `certs/azure-iot-test-only.root.ca.cert.pem` inside of `mycerts` folder.

Then select “Save”

![](doc/images/azure_iot_hub_setup_x509_03.png)

Now there is a new certificate in your certificate list. Its status is “unverified”, and you need to prove possession.

![](doc/images/azure_iot_hub_setup_x509_04.png)

To prove of possession, select your certificate. It’ll pop-up a menu at right side. Select “Generate Verification Code”.

![](doc/images/azure_iot_hub_setup_x509_05.png)

A verification code is generated. Then you need to sign this string like below:

```
./certGen.sh create_verification_certificate <Your Verification Code>
```

The pem file of the signed string is placed in `certs/verification-code.cert.pem`. You need to upload this file into the field of “Verification Certificate .pem or .cer file.” Then select “Verify“.

![](doc/images/azure_iot_hub_setup_x509_06.png)

The status of your certificate should be “Verified”.

![](doc/images/azure_iot_hub_setup_x509_07.png)

Now you can create a certificate for your device. The last parameter is your device ID.

```
./certGen.sh create_device_certificate MyIotThing
```

It will creates device’s certificate in `certs/new-device.cert.pem`, and  private key in `private/new-device.key.pem`. Now we can use this certificate and private key to make a x.509 connection.

## Run x.509 sample code

Use a text editor to edit x.509 sample code from Azure IoT Device SDK for C. Here is the file name: `iothub_client/samples/iothub_ll_client_x509_sample/iothub_ll_client_x509_sample.c`.

There are configuration parameters to fill in. The first one is `connectionString` . Here is an example:

```c
static const char* connectionString = "HostName=my-things.azure-devices.net;DeviceId=MyIotThing;x509=true";
```

In the connection string, the HostName has the format `{IoT-Hub name}.azure-devices.net`. And you also need to fill in your device ID.

Then you need to fill the content of ``certs/new-device.cert.pem` into `x509certificate`,  and fill the content of `private/new-device.key.pem` into `x509privatekey`. **IMPORTANT: Please add end of line character "\n" at the end of each line in certificate or private key.**

```c
static const char* x509certificate =
"-----BEGIN CERTIFICATE-----""\n"
"MIICpDCCAYwCCQCfIjBnPxs5TzANBgkqhkiG9w0BAQsFADAUMRIwEAYDVQQDDAls""\n"
"b2NhbGhvc3QwHhcNMTYwNjIyMjM0MzI3WhcNMTYwNjIzMjM0MzI3WjAUMRIwEAYD""\n"
"...""\n"
"+s88wBF907s1dcY45vsG0ldE3f7Y6anGF60nUwYao/fN/eb5FT5EHANVMmnK8zZ2""\n"
"tjWUt5TFnAveFoQWIoIbtzlTbOxUFwMrQFzFXOrZoDJmHNWc2u6FmVAkowoOSHiE""\n"
"dkyVdoGPCXc=""\n"
"-----END CERTIFICATE-----";

static const char* x509privatekey =
"-----BEGIN RSA PRIVATE KEY-----""\n"
"MIIEpQIBAAKCAQEA0zKK+Uu5I0nXq2V6+2gbdCsBXZ6j1uAgU/clsCohEAek1T8v""\n"
"qj2tR9Mz9iy9RtXPMHwzcQ7aXDaz7RbHdw7tYXqSw8iq0Mxq2s3p4mo6gd5vEOiN""\n"
"...""\n"
"EyePNmkCgYEAng+12qvs0de7OhkTjX9FLxluLWxfN2vbtQCWXslLCG+Es/ZzGlNF""\n"
"SaqVID4EAUgUqFDw0UO6SKLT+HyFjOr5qdHkfAmRzwE/0RBN69g2qLDN3Km1Px/k""\n"
"xyJyxc700uV1eKiCdRLRuCbUeecOSZreh8YRIQQXoG8uotO5IttdVRc=""\n"
"-----END RSA PRIVATE KEY-----";
```

After everything is done, go to cmake folder, build solution, and run:

```
cd cmake
cmake --build . -- -j4
./iothub_client/samples/iothub_ll_client_x509_sample/iothub_ll_client_x509_sample
```

The execution result would be something like this:

```
# ./iothub_client/samples/iothub_ll_client_x509_sample/iothub_ll_client_x509_sample
Creating IoTHub handle
Sending message 1 to IoTHub
Sending message 2 to IoTHub
Sending message 3 to IoTHub
Sending message 4 to IoTHub
Sending message 5 to IoTHub
Confirmation callback received for message 1 with result IOTHUB_CLIENT_CONFIRMATION_OK
Confirmation callback received for message 2 with result IOTHUB_CLIENT_CONFIRMATION_OK
Confirmation callback received for message 3 with result IOTHUB_CLIENT_CONFIRMATION_OK
Confirmation callback received for message 4 with result IOTHUB_CLIENT_CONFIRMATION_OK
Confirmation callback received for message 5 with result IOTHUB_CLIENT_CONFIRMATION_OK
Press any key to continue
```

# Setup AWS IoT Core to use the same x.509 certificate

AWS IoT Core supports client certificates signed by custom certificate authorities (CA). So let’s add the same root CA to IoT Core.

Go to AWS IoT core page: https://us-west-2.console.aws.amazon.com/iot/

Change to a proper region at the upper-right corner of the console.

![](doc/images/aws_iot_core_setup_ca_01.png)

## Register root CA

Select “CAs” in “Secure” menu at left side.

![](doc/images/aws_iot_core_setup_ca_02.png)

Select “Register CA”

![](doc/images/aws_iot_core_setup_ca_03.png)

Scroll down, select “Select CA certificate”. Upload the file `certs/azure-iot-test-only.root.ca.cert.pem` in your certificate folder.

![](doc/images/aws_iot_core_setup_ca_04.png)

Copy the string from Step 3 section. This string needs to be signed by the root CA following the steps below.

Go to your certificate folder. Input this command to sign this string:

```
./certGen.sh create_verification_certificate <Your Verification Code>
```
Now click “Select verification certificate” in Step 6. Upload the verification result `certs/verification-code.cert.pem` in your certificate folder.

Check “Activate CA certificate” and “Enable auto-registration of device certificates”. Then select “Register CA certificate”.

![](doc/images/aws_iot_core_setup_ca_05.png)

Now we have a root CA.

![](doc/images/aws_iot_core_setup_ca_06.png)

## Create Policy

Before we create thing. We need to create a policy first. Select “Policies” in “Secure” menu.

![](doc/images/aws_iot_core_setup_policy_01.png)

Select “Create a policy”

![](doc/images/aws_iot_core_setup_policy_02.png)

Input the policy name. Input `iot:*` in “Action”. Input `*` in “Resource ARN”. Check “Allow”. Select “Create”.  Note that this policy is not suitable for real applications because it’s too permissive.  This example is for demonstration purpose only.

![](doc/images/aws_iot_core_setup_policy_03.png)

Now our policy for thing is created.

![](doc/images/aws_iot_core_setup_policy_04.png)

## Create Thing

Select “Things” in “Manage” menu.

![](doc/images/aws_iot_core_setup_thing_01.png)

Select “Register a thing” to create thing.

![](doc/images/aws_iot_core_setup_thing_02.png)

Select “Create a single thing”

![](doc/images/aws_iot_core_setup_thing_03.png)

Input your thing name. Scroll down and select “Next”

![](doc/images/aws_iot_core_setup_thing_04.png)

Choose “Use my certificate”.

![](doc/images/aws_iot_core_setup_thing_05.png)

Select the CA that we just added. Then select “Next”.

![](doc/images/aws_iot_core_setup_thing_06.png)

Now we can select our own device certificate. Upload the certificate in `certs/new-device.cert.pem` . Check “Activate”. Then select “Next”.

![](doc/images/aws_iot_core_setup_thing_07.png)

Check the policy we just created. Then select “Register Thing”.

![](doc/images/aws_iot_core_setup_thing_08.png)

Now we’ve created a thing in AWS IoT Core with the same root CA and device certificate as in Azure IoT Hub.  You can use different ones of course, as long as you make corresponding changes in the firmware to match.

![](doc/images/aws_iot_core_setup_thing_09.png)

## Verify the certificate

We would need the endpoint of the thing we just created. Select thing.

![](doc/images/aws_iot_core_verify_thing_01.png)

Select “Interact” in left menu. There is a endpoint shown in HTTPS section. Keep this endpoint address, we will use it several times.

![](doc/images/aws_iot_core_verify_thing_02.png)

We can test our device certificate via command line as below. You need to replace your own endpoint address, device certificate and private key.

```
openssl s_client -connect abcd1234-ats.iot.us-west-2.amazonaws.com:8443 \
    -cert certs/new-device.cert.pem -key private/new-device.key.pem -state -debug
```

If everything works fine, then it’ll show below log and enter openssl client interactive mode.

```
SSL handshake has read 5401 bytes and written 2487 bytes
Verification: OK
---
New, TLSv1.2, Cipher is ECDHE-RSA-AES128-GCM-SHA256
Server public key is 2048 bit
Secure Renegotiation IS supported
Compression: NONE
Expansion: NONE
No ALPN negotiated
SSL-Session:
    Protocol  : TLSv1.2
    Cipher    : ECDHE-RSA-AES128-GCM-SHA256
    Session-ID: CDBC943523BC1AADCBC51A778D29384C37B896CD09C9413178330224B7E79506
    Session-ID-ctx:
    Master-Key: <key value>
    PSK identity: None
    PSK identity hint: None
    SRP username: None
    Start Time: 1592918082
    Timeout   : 7200 (sec)
    Verify return code: 0 (ok)
    Extended master secret: yes
---
```

# Apply patch to Azure IoT device SDK for C, run sample code to connect to AWS IoT Core

There is one file to patch. It’s locate at "[iothub_client/src/iothubtransport_mqtt_common.c](iothub_client/src/iothubtransport_mqtt_common.c)". Overwrite the file with the same name in your copy of the Azure IoT device SDK.

To understand how the patch works, please refer to "[Migration from Azure IoT Hub to AWS IoT Core.pdf](doc/Migration%20from%20Azure%20IoT%20Hub%20to%20AWS%20IoT%20Core.pdf)".

## Run x.509 sample code with patch

We have run x.509 sample code to connect to Azure IoT Hub. Now we can modify the connect string and make it connect to AWS IoT Core.

Use a text editor to modify sample code ``iothub_client/samples/iothub_ll_client_x509_sample/iothub_ll_client_x509_sample.c` . Modify `connectionString` as below. We change host address to endpoint address, and device ID to our thing name.

```c
static const char* connectionString = \
    "HostName=abcd1234-ats.iot.us-west-2.amazonaws.com;" \
    "DeviceId=MyIotThing;x509=true";
```

We don’t need to change `x509certificate` and `x509privatekey` because we use the same certificate and private key in this example.

Before running the sample, open the test console in AWS IoT Core to monitor MQTT messages.

Select “Test” in the left menu in AWS IoT Core.

![](doc/images/aws_iot_core_run_x509_sample_code_01.png)

Subscribe to topic `#` . It’s a wildcard that matches to all MQTT topics. Then select “Subscribe to topic”.

![](doc/images/aws_iot_core_run_x509_sample_code_02.png)

Build and run the x.509 example now. 

```
cd cmake
cmake --build . -- -j4
./iothub_client/samples/iothub_ll_client_x509_sample/iothub_ll_client_x509_sample
```

The execution result should be the same as running it on Azure IoT Hub before.

You can also see the results on AWS IoT Core test console:

![](doc/images/aws_iot_core_run_x509_sample_code_03.png)

## Run Device Twins example while it connects to AWS IoT Core Device Shadow

In Azure IoT device SDK, there is an example for testing device twins -  `iothub_client/samples/iothub_client_device_twin_and_methods_sample/iothub_client_device_twin_and_methods_sample.c` . After applying the patch, the SDK can interact with the AWS IoT Device Shadow.  

Modify the connection configuration, while keeping the same certificate and private key:

```c
static const char* connectionString = \
    "HostName=abcd1234-ats.iot.us-west-2.amazonaws.com;" \
    "DeviceId=MyIotThing;x509=true";

static const char* x509certificate =
"-----BEGIN CERTIFICATE-----\n" \
"MIIFvTCCA6WgAwIBAgIBAzANBgkqhkiG9w0BAQsFADAqMSgwJgYDVQQDDB9BenVy\n" \
"ZSBJb1QgSHViIENBIENlcnQgVGVzdCBPbmx5MB4XDTIwMDYyMzExMjc0OFoXDTIw\n" \
"......" \

"BYQ74AmnqR4EmDW/4XPrwGyLHitfKH9Rajf3h6mEbEtgGT1GHnT0ZcqZu3E0bXzH\n" \
"fzEp435tkmvxI2+FGHtEs1T+UKkxFyHV3l6UWC2GIpA4\n" \
"-----END CERTIFICATE-----\n";

static const char* x509privatekey =
"-----BEGIN RSA PRIVATE KEY-----\n" \
"MIIJKQIBAAKCAgEA4qBSfrC6NQkexbNyH+ouwsJz/+kTrO3Z4HssVzdbpVIwZfGn\n" \
"4UJm5KKJWD7pOjCX61UATgRywcTUY8BtftTtV+xVzLhMc4KaSJNZ/+11yDph4CMf\n" \
"......" \
"nRlE3DmF4eIRmtnHos/zSjgL0qXVPq65itSTdMZQL/YldMnyEx7j9aGOPve2Phji\n" \
"8BIQRnW4h7VvtZJO6WqGfxmvMNAXmJAfcFnsgxqrHsvpmsOdG/QrU/DXDe6x\n" \
"-----END RSA PRIVATE KEY-----\n";
```

To tell the SDK to use x.509 certificate for the connection, we need to add calls to `IoTHubDeviceClient_SetOption` like below:

```c
// Uncomment the following lines to enable verbose logging (e.g., for debugging).
//bool traceOn = true;
//(void)IoTHubDeviceClient_SetOption(iotHubClientHandle, OPTION_LOG_TRACE, &traceOn);

#ifdef SET_TRUSTED_CERT_IN_SAMPLES
// For mbed add the certificate information
if (IoTHubDeviceClient_SetOption(iotHubClientHandle, "TrustedCerts", certificates) != IOTHUB_CLIENT_OK)
{
    (void)printf("failure to set option \"TrustedCerts\"\r\n");
}
#endif // SET_TRUSTED_CERT_IN_SAMPLES

if (
    (IoTHubDeviceClient_SetOption(iotHubClientHandle, OPTION_X509_CERT, x509certificate) != IOTHUB_CLIENT_OK) ||
    (IoTHubDeviceClient_SetOption(iotHubClientHandle, OPTION_X509_PRIVATE_KEY, x509privatekey) != IOTHUB_CLIENT_OK)
    )
{
    printf("failure to set options for x509, aborting\r\n");
}

Car car;
memset(&car, 0, sizeof(Car));
```

Now we can build and test this example.

```
cd cmake
cmake --build . -- -j4
./iothub_client/samples/iothub_client_device_twin_and_methods_sample/iothub_client_device_twin_and_methods_sample
```

After execution you can see these logs in the console.

```
Info: original topic_resp: $aws/things/MyIotThing/shadow/get/accepted

Info: modified topic_resp: $iothub/twin/res/200/?$rid=2

Info: original payload(705): {"state":{},"metadata":{},"version":3,"timestamp":1592924027}

Info: modified payload(252): {"desired":{"$version":3},"reported":{"$version":3}}

GetTwinAsync result:
{"reported":{"lastOilChangeDate":"2016","maker":{"makerName":"Fabrikam","style":"sedan","year":2014},"state":{"reported_maxSpeed":100,"softwareVersion":1,"vanityPlate":"1I1"},"$version":13}}
Info: original topic_resp: $aws/things/MyIotThing/shadow/update/documents

Info: modified topic_resp: null

Info: original topic_resp: $aws/things/MyIotThing/shadow/update/delta

Info: modified topic_resp: null

Info: original topic_resp: $aws/things/MyIotThing/shadow/update/documents

Info: modified topic_resp: null

Info: original topic_resp: $aws/things/MyIotThing/shadow/update/accepted

Info: modified topic_resp: $iothub/twin/res/200/?$rid=4

Info: original payload(542): {"state":{"reported":{"lastOilChangeDate":"2016","maker":{"makerName":"Fabrikam","style":"sedan","year":2014},"state":{"reported_maxSpeed":100,"softwareVersion":1,"vanityPlate":"1I1"}}},"metadata":{"reported":{"lastOilChangeDate":{"timestamp":1592926716},"maker":{"makerName":{"timestamp":1592926716},"style":{"timestamp":1592926716},"year":{"timestamp":1592926716}},"state":{"reported_maxSpeed":{"timestamp":1592926716},"softwareVersion":{"timestamp":1592926716},"vanityPlate":{"timestamp":1592926716}}}},"version":14,"timestamp":1592926716}

Info: modified payload(216): {"desired":{"$version":14},"reported":{"lastOilChangeDate":"2016","maker":{"makerName":"Fabrikam","style":"sedan","year":2014},"state":{"reported_maxSpeed":100,"softwareVersion":1,"vanityPlate":"1I1"},"$version":14}}

Device Twin reported properties update completed with result: 200
```

This sample code will get and update the device shadow when it runs. To check whether it works, select the thing.

![](doc/images/aws_iot_core_run_device_twins_sample_code_01.png)

Select “Shadows”. And you can see the reported state from device.

![](doc/images/aws_iot_core_run_device_twins_sample_code_02.png)

Now let’s modify shadow state manually and check how device responses.

Select “Edit” at the right side.

![](doc/images/aws_iot_core_run_device_twins_sample_code_03.png)

Modify shadow state as below. We add a desired state for desired_maxSpeed.

```
{
  "desired": {
    "settings": {
      "desired_maxSpeed": 120
    }
  },
  "reported": {
    "lastOilChangeDate": "2016",
    "maker": {
      "makerName": "Fabrikam",
      "style": "sedan",
      "year": 2014
    },
    "state": {
      "reported_maxSpeed": 100,
      "softwareVersion": 1,
      "vanityPlate": "1I1"
    }
  }
}
```

Then select “Save”.

![](doc/images/aws_iot_core_run_device_twins_sample_code_04.png)

Then you can see device getting the `desired_maxSpeed` desired state.

```
Info: original topic_resp: $aws/things/MyIotThing/shadow/update/documents

Info: modified topic_resp: $iothub/twin/PATCH/properties/desired

Info: original payload(1319): {"previous":{"state":{"desired":{"settings":{"desired_maxSpeed":120}},"reported":{"lastOilChangeDate":"2016","maker":{"makerName":"Fabrikam","style":"sedan","year":2014},"state":{"reported_maxSpeed":100,"softwareVersion":1,"vanityPlate":"1I1"}}},"metadata":{"desired":{"settings":{"desired_maxSpeed":{"timestamp":1592926073}}},"reported":{"lastOilChangeDate":{"timestamp":1592926716},"maker":{"makerName":{"timestamp":1592926716},"style":{"timestamp":1592926716},"year":{"timestamp":1592926716}},"state":{"reported_maxSpeed":{"timestamp":1592926716},"softwareVersion":{"timestamp":1592926716},"vanityPlate":{"timestamp":1592926716}}}},"version":14},"current":{"state":{"desired":{"settings":{"desired_maxSpeed":120}},"reported":{"lastOilChangeDate":"2016","maker":{"makerName":"Fabrikam","style":"sedan","year":2014},"state":{"reported_maxSpeed":100,"softwareVersion":1,"vanityPlate":"1I1"}}},"metadata":{"desired":{"settings":{"desired_maxSpeed":{"timestamp":1592926795}}},"reported":{"lastOilChangeDate":{"timestamp":1592926795},"maker":{"makerName":{"timestamp":1592926795},"style":{"timestamp":1592926795},"year":{"timestamp":1592926795}},"state":{"reported_maxSpeed":{"timestamp":1592926795},"softwareVersion":{"timestamp":1592926795},"vanityPlate":{"timestamp":1592926795}}}},"version":15},"timestamp":1592926795}

Info: modified payload(240): {"settings":{"desired_maxSpeed":120},"$version":15},"reported":{"lastOilChangeDate":"2016","maker":{"makerName":"Fabrikam","style":"sedan","year":2014},"state":{"reported_maxSpeed":100,"softwareVersion":1,"vanityPlate":"1I1"},"$version":15}

Info: original topic_resp: $aws/things/MyIotThing/shadow/update/delta

Info: modified topic_resp: null

Received a new desired_maxSpeed = 120
Info: original topic_resp: $aws/things/MyIotThing/shadow/update/accepted

Info: modified topic_resp: null
```

## Verify Direct Methods on AWS IoT Core

You can also test direct methods by exchanging MQTT messages.

Go to test console. Subscribe topic `#` to monitor MQTT messages. Then input topic `device/MyIotThing/methods/getCarVIN/1234abcd1234abcd` in Publish section.  The method name is `getCarVIN` . The numbers append are request id and they can be random numbers for test purpose. Now we select “Publish to topic”.

![](doc/images/aws_iot_core_run_device_twins_sample_code_05.png)

On the console you should see two MQTT messages. The first one is the what you just published. The second is the response from device.

![](doc/images/aws_iot_core_run_device_twins_sample_code_06.png)

## Verify Direct Methods over HTTP

As described in [Azure IoT Hub Direct Method](https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-devguide-direct-methods), you can verify direct methods over HTTP. This project includes an AWS Lambda serverless program that you can deploy into your AWS account for handling HTTP calls of direct methods.  Please see to [Direct Method](../direct_method/README.md)for details.

## Go back to Azure IoT for debugging

If you want to go back to connect to Azure IoT Hub, just disable the compile option `IOT_CORE_PATCH` in `iothub_client/src/iothubtransport_mqtt_common.c` .

```c
//#define IOT_CORE_PATCH
```

You also need to change the connection string in your sample code, then rebuild.

# Optional Features

## Subscribe and publish custom topics

The Azure IoT SDK for C doesn’t support publishing and subscribing to arbitrary MQTT topics.  The patch in this project provides the capability of subscribing to arbitrary topics.  This support is enabled by default in the patch.  You can disable it by commenting out the line that defines “**IOT_CORE_PATCH_SUPPORT_SUB_PUB**” at the beginning of iothubtransport_mqtt_common.c.

Here is an example of how to publish to a custom topic.

```
MQTT_MESSAGE_HANDLE mqttMsg = mqttmessage_create(0, "mytopic", DELIVER_AT_LEAST_ONCE, "Hello World!", sizeof("Hello World") - 1);
IoTHubDeviceClient_LL_SetOption(device_ll_handle, "mqtt_publish", mqttMsg);
mqttmessage_destroy(mqttMsg);
```

To subscribe to a custom topic, you need to register a message callback first.

```
IoTHubClientCore_LL_SetMessageCallback(device_ll_handle, messageCallback, NULL);
```

Then you can subscribe to a custom topic:

```
IoTHubDeviceClient_LL_SetOption(device_ll_handle, "mqtt_subscribe", "mytopic");
```

In your callback handler, you can get the topic and payload like this:

```
static IOTHUBMESSAGE_DISPOSITION_RESULT messageCallback(IOTHUB_MESSAGE_HANDLE message, void* user_context)
{
    const unsigned char* buff_msg;
    size_t buff_len;

    const char* topic = IoTHubMessage_GetProperty(message, "topic");
    if (topic != NULL)
    {
        printf("topic: %s\r\n", topic);
    }

    if (IoTHubMessage_GetByteArray(message, &buff_msg, &buff_len) == IOTHUB_MESSAGE_OK)
    {
        printf("Payload(%d): \r\n%.*s\r\n", (int)buff_len, (int)buff_len, buff_msg);
    }

    return IOTHUBMESSAGE_ACCEPTED;
}
```


----------------------------------------------------------------

# FAQ

## MQTT payload size limitation caused by MBEDTLS_SSL_MAX_FRAGMENT_LENGTH

mbedTLS supports [RFC 6066](https://tools.ietf.org/html/rfc6066#section-4), Maximum Fragment Size Negotiation. When you set mbedTLS’ MBEDTLS_SSL_MAX_FRAGMENT_LENGTH to a value smaller than 16384 on the device, the upstream fragments’ sizes will not exceed it.  However, AWS IoT core does not support RFC 6066 when this FAQ is published. It sets its maximum fragment size to 16384 and has no guarantees for any size below that.  Therefore, the size of the downstream fragments is not guaranteed to be what you intend to negotiate. 

To make it worse, when the application publishes a document to the device shadow’s “update” topic, IoT Core will publish to the “update/accepted” and “update/documents” topics with documents of bigger sizes, which include meta data. Here is an example.

When the application sends this payload to the “update” topic:

```json
{
    "state": {
        "reported": {
            "color": "red",
            "power": false
        }
    }
}
```

IoT Core publishes to the “update/accepted” topic with a document that can be 2x more bigger than the original.

```json
{
    "state": {
        "reported": {
            "color": "red",
            "power": false
        }
    },
    "metadata": {
        "reported": {
            "color": {
                "timestamp": 1594963236
            },
            "power": {
                "timestamp": 1594963236
            }
        }
    },
    "version": 7,
    "timestamp": 1594963236
}
```

It also publishes to the “update/documents” topic with a document that can be 4x more bigger than the original.

```json
{
    "previous": {
        "state": {
            "reported": {
                "color": "red",
                "power": true
            }
        },
        "metadata": {
            "reported": {
                "color": {
                    "timestamp": 1594963176
                },
                "power": {
                    "timestamp": 1594963176
                }
            }
        },
        "version": 6
    },
    "current": {
        "state": {
            "reported": {
                "color": "red",
                "power": false
            }
        },
        "metadata": {
            "reported": {
                "color": {
                    "timestamp": 1594963236
                },
                "power": {
                    "timestamp": 1594963236
                }
            }
        },
        "version": 7
    },
    "timestamp": 1594963236
}
```

Below are some workarounds to this issue:
1.  Use a fragment size of 16384 if the device memory can accommodate.  This is the safest approach.
2.  Consider using asymmetric Tx/Rx buffer sizes.  See [Allow asymmetric in/out buffer lengths to conserve RAM](https://github.com/ARMmbed/mbedtls/pull/536). It allows  setting separate MBEDTLS_SSL_IN_CONTENT_LEN and MBEDTLS_SSL_OUT_CONTENT_LEN.
3.  Use ECDSA cert chains. ECDSA based cipher suites are much smaller and the overall cert chain is smaller. 
4.  Keep application payload sizes small. The patch to Azure SDK included in this project does not subscribe to the update/documents topic of the AWS IoT Core device shadows; it subscribes to the update/accepted topic. Experiments showed that if the application’s shadow payload sizes were kept below 1/3 of MBEDTLS_SSL_MAX_FRAGMENT_LENGTH, the downstream fragment would not exceed MBEDTLS_SSL_MAX_FRAGMENT_LENGTH.  The exact factor depend on the specific application’s payload though, therefore you should determine the proper factor by sufficient testing.