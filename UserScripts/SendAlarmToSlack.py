# Sam Grant 2024
# Send messages to Slack 

import argparse
import subprocess

def SendAlarmToSlack(message, url="https://hooks.slack.com/services/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"):
        try:
                # Construct the curl command to send the message to Slack
                command = f"curl -X POST -H 'Content-type: application/json' --data '{{\"text\":\"{message}\"}}' {url}"
                # Execute the curl command using subprocess
                subprocess.run(command, shell=True, check=True)
                print("Message sent to Slack successfully")
        except subprocess.CalledProcessError as error:
                print(f"Error sending message to Slack: {error}")

        return

def main():
        # Create an ArgumentParser object
        parser = argparse.ArgumentParser() 

        # Add optional argument for message
        parser.add_argument("--message") 

        # Parse the command-line arguments
        args = parser.parse_args()

        # Print the parsed arguments
        # print('Message:', args.message)

        SendAlarmToSlack(args.message)

if __name__ == '__main__':
        main()

