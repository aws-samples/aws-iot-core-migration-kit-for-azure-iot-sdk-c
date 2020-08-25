# DirectMethodApi

This project contains source code and supporting files for an AWS serverless application that allows you to migrate client applications of the [Azure IoT Hub Direct Method](https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-devguide-direct-methods) to AWS.

This application can be deployed into your AWS account by using the [AWS Serverless Application Model](https://docs.aws.amazon.com/serverless-application-model/latest/developerguide/what-is-sam.html) Command Line Interface ([AWS SAM CLI](https://docs.aws.amazon.com/serverless-application-model/latest/developerguide/serverless-sam-cli-install.html).) It includes the following files and folders.

* `direct_method` - Code for the application's Lambda function.
* `template.yaml` - A template that defines the application's AWS resources.

The application uses several AWS resources, including Lambda functions and an API Gateway API. These resources are defined in the `template.yaml` file in this project. You can update the template to add AWS resources when you customize this application.

If you prefer to use an integrated development environment (IDE) to build and test your application, you can use the AWS Toolkit.  The AWS Toolkit is an open source plug-in for popular IDEs that uses the SAM CLI to build and deploy serverless applications on AWS. The AWS Toolkit also adds a simplified step-through debugging experience for Lambda function code. See the following links to get started.

* [PyCharm](https://docs.aws.amazon.com/toolkit-for-jetbrains/latest/userguide/welcome.html)
* [IntelliJ](https://docs.aws.amazon.com/toolkit-for-jetbrains/latest/userguide/welcome.html)
* [VS Code](https://docs.aws.amazon.com/toolkit-for-vscode/latest/userguide/welcome.html)
* [Visual Studio](https://docs.aws.amazon.com/toolkit-for-visual-studio/latest/user-guide/welcome.html)

## Download AWS IoT endpoint CA certificate
Before deploying the API, one should first download the AWS IoT endpoint CA certificate to the project. This CA certificate is required for the verification of the server endpoint. You may download one of the CA certificates from [here](https://docs.aws.amazon.com/iot/latest/developerguide/server-authentication.html#server-authentication-certs), and put it into the `src` directory. During the deployment, you will be asked to provide the endpoint, which should match your chosen CA certificate. For more information, please refer to the [Server Authentication documentation](https://docs.aws.amazon.com/iot/latest/developerguide/server-authentication.html).

## Deploy the sample application

The Serverless Application Model Command Line Interface (SAM CLI) is an extension of the AWS CLI that adds functionality for building and testing Lambda applications. It uses Docker to run your functions in an Amazon Linux environment that matches Lambda. It can also emulate your application's build environment and API.

To use the SAM CLI, you need the following tools.


* SAM CLI - [Install the SAM CLI](https://docs.aws.amazon.com/serverless-application-model/latest/developerguide/serverless-sam-cli-install.html)
* [Python 3 installed](https://www.python.org/downloads/)
* Docker - [Install Docker community edition](https://hub.docker.com/search/?type=edition&offering=community)

To build and deploy your application for the first time, run the following in your shell:


```
sam build --use-container
sam deploy --guided
```


The first command will build the source of your application. The second command will package and deploy your application to AWS, with a series of prompts:


* **Stack Name**: The name of the stack to deploy to CloudFormation. This should be unique to your account and region, and a good starting point would be something matching your project name.
* **AWS Region**: The AWS region you want to deploy your app to.
* **Parameter IotEndpoint**: The AWS IoT Endpoint. One can get the ATS-signed endpoint from the Settings page of the AWS IoT Core Console.
* **Confirm changes before deploy**: If set to yes, any change sets will be shown to you before execution for manual review. If set to no, the AWS SAM CLI will automatically deploy application changes.
* **Allow SAM CLI IAM role creation**: Many AWS SAM templates, including this example, create AWS IAM roles required for the AWS Lambda function(s) included to access AWS services. By default, these are scoped down to minimum required permissions. To deploy an AWS CloudFormation stack which creates or modified IAM roles, the `CAPABILITY_IAM` value for `capabilities` must be provided. If permission isn't provided through this prompt, to deploy this example you must explicitly pass `--capabilities CAPABILITY_IAM` to the `sam deploy` command.
* **Save arguments to samconfig.toml**: If set to yes, your choices will be saved to a configuration file inside the project, so that in the future you can just re-run `sam deploy` without parameters to deploy changes to your application.

You can find your API Gateway Endpoint URL in the output values displayed after deployment.


## Test the Direct Method API with API Gateway and IoT Core Console

### Subscribe to Direct Method Request Topic

1. Navigate to the [IoT Core console](https://console.aws.amazon.com/iot/home), click "Test" on the left menu.
2. Enter `device/<myThingName>/methods/<myMethodName>/+` under "Subscription topic" and click "Subscribe to topic".
3. Leave the browser tab open as we will respond to the Direct Method here later.

### Fire the Direct Method API

1. Navigate to the [API Gateway console](https://console.aws.amazon.com/apigateway/home), click on the API you have just deployed with SAM CLI.
2. Click the POST action under "Resources".
3. Click the "Test" button on the right panel.
4. Enter a `<myThingName>` in the `{thingName}` field under "Path".
5. Enter `Content-Type: application/json` in the "Headers" textbox.
6. Enter a request body in the following format.
```
{
  "methodName": "<myMethodName>",
  "responseTimeoutInSeconds": 200,
  "payload": {
      "input1": "someInput",
      "input2": "anotherInput"
  }
}
```

7. Click "Test". Leave this page open as we will come back to verify the API response.

### Respond to the Direct Method

1. Go back to the IoT Core console Test page where you have just subscribed to the Direct Method request topic.
2. You should see a message like this one from the topic `device/<myThingName>/method/<myMethodName>/<requestId>`:
```
{
  "input1": "someInput",
  "input2": "anotherInput"
}
```

3. Enter `device/methods/res/200/<requestId>` under "Publish". The `<requestId>` should be the same as the one from the request message above.
4. Leave the payload as the default one `{"message": "Hello from AWS IoT console"}` and click "Publish to topic".
5. Go back to the API Gateway Test page and you should see an API response is received with status code 200 and payload `{"message": "Hello from AWS IoT console"}`.

## Use the SAM CLI to build and test locally

Build your application with the `sam build --use-container` command.


```
DirectMethodApi$ sam build --use-container
```


The SAM CLI installs dependencies defined in `direct_method/requirements.txt`, creates a deployment package, and saves it in the `.aws-sam/build` folder.

Test a single function by invoking it directly with a test event. An event is a JSON document that represents the input that the function receives from the event source. Test events are included in the `events` folder in this project.

Run functions locally and invoke them with the `sam local invoke` command.


```
DirectMethodApi$ sam local invoke DirectMethodFunction --event events/event.json
```


The SAM CLI can also emulate your application's API. Use the `sam local start-api` to run the API locally on port 3000.


```
DirectMethodApi$ sam local start-api
DirectMethodApi$ curl http://localhost:3000/
```


The SAM CLI reads the application template to determine the API's routes and the functions that they invoke. The `Events` property on each function's definition includes the route and method for each path.


```
      Events:
        DirectMethodApi:
          Type: Api
          Properties:
            Path: '/thing/{thingName}'
            Method: POST
```



## Add a resource to your application

The application template uses AWS Serverless Application Model (AWS SAM) to define application resources. AWS SAM is an extension of AWS CloudFormation with a simpler syntax for configuring common serverless application resources such as functions, triggers, and APIs. For resources not included in [the SAM specification](https://github.com/awslabs/serverless-application-model/blob/master/versions/2016-10-31.md), you can use standard [AWS CloudFormation](https://docs.aws.amazon.com/AWSCloudFormation/latest/UserGuide/aws-template-resource-type-ref.html) resource types.


## Fetch, tail, and filter Lambda function logs

To simplify troubleshooting, SAM CLI has a command called `sam logs`. `sam logs` lets you fetch logs generated by your deployed Lambda function from the command line. In addition to printing the logs on the terminal, this command has several nifty features to help you quickly find the bug.

`NOTE`: This command works for all AWS Lambda functions; not just the ones you deploy using SAM.


```
DirectMethodLambda$ sam logs -n DirectMethodFunction --stack-name DirectMethodLambda --tail
```


You can find more information and examples about filtering Lambda function logs in the [SAM CLI Documentation](https://docs.aws.amazon.com/serverless-application-model/latest/developerguide/serverless-sam-cli-logging.html).


## Cleanup

To delete the sample application that you created, use the AWS CLI. Assuming you used your project name for the stack name, you can run the following:


```
aws cloudformation delete-stack --stack-name DirectMethodLambda
```



## Resources

See the [AWS SAM developer guide](https://docs.aws.amazon.com/serverless-application-model/latest/developerguide/what-is-sam.html) for an introduction to SAM specification, the SAM CLI, and serverless application concepts.

