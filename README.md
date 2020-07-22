# AWS IoT Migration Kit for Azure Applications

This kit allows developers to migrate IoT applications from Azure to AWS.  It provides the following components:


1.  A software patch to the [Azure IoT C SDK](https://github.com/Azure/azure-iot-sdk-c).  This patch modifies the MQTT topics that the device sends and receives [Device Twin](https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-devguide-device-twins) messages to and from, so that when the device connects to AWS IoT, the messages will be directed to the [AWS IoT Device Shadow](https://docs.aws.amazon.com/iot/latest/developerguide/iot-device-shadows.html).  
Wherever the payload formats of Azure Device Twin and AWS Device Shadow have a difference, this patch does the conversion so the application code can stay the same.  This patch also provides a way for re-configuring the MQTT topics of the Azure [Direct Methods](https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-devguide-direct-methods), so that the application code on the device that handles direct methods can work as is.  
With the assistance of this patch, in most cases, the application code running on top of Azure IoT C SDK does not need any modification, and the messages it sends to and received from the cloud will stay the same regardless the device is connected to Azure or AWS.  See the [Differences between Device Twin and Device Shadow](iothub_client_src/doc/Migration%20from%20Azure%20IoT%20Hub%20to%20AWS%20IoT%20Core.pdf) for a conceptual summary.  Also see the [Device Migration Guide](iothub_client_src/README.md) for a step-by-step guidance. 
2.  An optional serverless application that can be easily deployed into your AWS account.  This serverless application is useful when your application uses the [Direct Methods](https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-devguide-direct-methods) of Azure and you need it work in the same way on AWS.  
This serverless application runs in AWS and exposes the same HTTP API for your client applications to invoke the direct methods implemented by the device.  See the [deployment guide](direct_method/README.md) for how to use this serverless application.
