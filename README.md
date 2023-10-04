Certainly, here's a README.md for your repository, which includes information about the "runner" code and how it's used with the CodeLand Manager:

```markdown
# CodeLand Runner

## Introduction

CodeLand Runner is a lightweight HTTP server designed to execute code snippets and return the results. It is primarily used in conjunction with the CodeLand Manager project for managing code execution on remote runners.

The CodeLand Runner provides a simple HTTP interface for executing code snippets and retrieving the results in a structured JSON format. It can be easily integrated into various systems that require remote code execution capabilities.

This repository contains the CodeLand Runner code, which is a part of the CodeLand Manager ecosystem.

## Usage

To use the CodeLand Runner, you'll typically follow these steps:

1. **Compile the Code**: Compile the CodeLand Runner code using the provided build script or your preferred method. Ensure that all dependencies, including the cJSON library, are correctly set up.

2. **Run the Server**: Start the CodeLand Runner server on the desired host and port. By default, it listens on port 15000, but you can customize it by setting the `runnerPort` environment variable.

3. **Send Requests**: Use HTTP requests to send code snippets to the CodeLand Runner for execution. You can send POST requests with a JSON payload containing the code to be executed. The runner will execute the code and return the results.

4. **Receive Results**: Parse the JSON response from the CodeLand Runner to retrieve the execution results. The response includes a "res" field containing the base64-encoded output of the executed code.

## Example

Here's an example of sending a POST request to execute code with the CodeLand Runner using `curl`:

```bash
curl -X POST http://localhost:15000 -d '{"code": "echo \"Hello, CodeLand!\"" }'
```

The response will be a JSON object with the execution result:

```json
{
  "res": "SGVsbG8sIENvZGVMYW5kIQo="
}
```

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Author

- William Mantly (wmantly@gmail.com)

---

*This README was generated with ❤️ by an AI language model.*
```
