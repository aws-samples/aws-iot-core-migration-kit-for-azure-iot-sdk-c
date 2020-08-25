import logging
import time
import json
import uuid
import threading
import os

from AWSIoTPythonSDK.MQTTLib import AWSIoTMQTTClient


logger = logging.getLogger("DirectMethodLambdaFunction")
logger.setLevel(logging.INFO)

clientId = 'directMethod_' +str(uuid.uuid4())
iotEndpoint = os.environ.get('IOT_ENDPOINT', None)
if not iotEndpoint:
    raise ValueError('IOT_ENDPOINT env variable is empty or missing!')

myAWSIoTMQTTClient = AWSIoTMQTTClient(clientId, useWebsocket=True)
myAWSIoTMQTTClient.configureEndpoint(iotEndpoint, 443)
myAWSIoTMQTTClient.configureCredentials('AmazonRootCA1.pem')

# AWSIoTMQTTClient connection configuration
myAWSIoTMQTTClient.configureAutoReconnectBackoffTime(1, 32, 20)
myAWSIoTMQTTClient.configureOfflinePublishQueueing(0)  # Infinite offline Publish queueing
myAWSIoTMQTTClient.configureConnectDisconnectTimeout(10)  # 10 sec
myAWSIoTMQTTClient.configureMQTTOperationTimeout(5)  # 5 sec


# Connect and subscribe to AWS IoT
myAWSIoTMQTTClient.connect()

REQUEST_TOPIC_FORMAT = 'device/%s/methods/%s/%s'
RESPONSE_TOPIC_FORMAT = 'device/methods/res/+/%s'

def generateCallback(apiResponse, responseTopic, deviceResponded):
    def customCallback(client, userdata, message):
        myAWSIoTMQTTClient.unsubscribeAsync(responseTopic)

        statusCode = int(message.topic.split('/')[-2])
        payload = message.payload.decode('utf-8')

        logger.info("Received direct method response: %s", payload)
        logger.info("Received from topic: %s", message.topic)
        logger.info("Direct response method status code: %d", statusCode)

        with deviceResponded:
            apiResponse['statusCode'] = statusCode
            apiResponse['body'] = payload
            deviceResponded.notifyAll()
        return

    return customCallback



def lambda_handler(event, context):
    requestId = str(uuid.uuid4())
    apiResponse = {'headers': {'Content-Type': 'application/json'}}

    deviceResponded = threading.Condition()
    mqttResponseTopic = RESPONSE_TOPIC_FORMAT % requestId

    logger.info('Subscribing to response topic: %s', mqttResponseTopic)
    myAWSIoTMQTTClient.subscribeAsync(mqttResponseTopic, 1, None, generateCallback(apiResponse, mqttResponseTopic, deviceResponded))

    thingName = event['pathParameters']['thingName']
    apiRequestBody = json.loads(event['body'])
    logger.info('Direct Method API request body: %s', apiRequestBody)

    methodName = apiRequestBody['methodName']
    responseTimeoutSeconds = float(apiRequestBody['responseTimeoutInSeconds'])
    methodPayload = apiRequestBody['payload']

    mqttRequestTopic = REQUEST_TOPIC_FORMAT % (thingName, methodName, requestId)
    logger.info('Publishing to request topic: %s', mqttResponseTopic)
    myAWSIoTMQTTClient.publishAsync(mqttRequestTopic, json.dumps(methodPayload), 1)


    with deviceResponded:
        if 'statusCode' not in apiResponse:
            deviceResponded.wait(responseTimeoutSeconds)

    if 'statusCode' not in apiResponse:
        logger.warning('Request timed out!')
        apiResponse['statusCode'] = 504
        apiResponse['body'] = json.dumps({'message': 'request timed out!'})

    return apiResponse

