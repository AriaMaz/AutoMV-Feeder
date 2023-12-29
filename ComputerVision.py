import cv2  # OpenCV for image processing
import base64  # For encoding images to base64
import openai  # OpenAI's API for machine learning models
import os  # To access environment variables
import threading  # For concurrent execution
import time  # For handling time-related functions
import serial  # For serial communication with microcontroller

# Constants
API_CALL_INTERVAL = 60 * 60 * 24 / 28800  # Time interval for API calls

# Establish a serial connection
ser = serial.Serial('COM3', 9600, timeout=1)  # Set up serial port connection
ser.flush()  # Clear the serial buffer

def encode_image_to_base64(image_path):
    # Function to convert an image to base64 encoding
    with open(image_path, "rb") as image_file:
        return base64.b64encode(image_file.read()).decode("utf-8")

def call_openai_api(ref_image1_base64, ref_image2_base64, current_image_base64):
    # Function to call OpenAI API for image comparison
    try:
        prompt_message = {
            # Defining the prompt for the GPT model
            "role": "user",
            "content": [
                # Instructions for the AI to follow
                "Given two reference images of two different pets, check if ONLY one of these pets appears in the second image. Respond with '1' if only pet 1 appears, '2' if only pet 2 appears, and '0' otherwise.",
                {"image": ref_image1_base64, "resize": 768},
                {"image": ref_image2_base64, "resize": 768},
                {"image": current_image_base64, "resize": 768}
            ],
        }

        params = {
            "model": "gpt-4-vision-preview",
            "messages": [prompt_message],
            "api_key": os.environ["OPENAI_API_KEY"],
            "headers": {"Openai-Version": "2020-11-07"},
            "max_tokens": 500,    #Can be varried based on the amount of time you want to run the vision model and the amount of API tokens you want to use
        }

        result = openai.ChatCompletion.create(**params)
        api_response = result.choices[0].message.content.strip()
        print(api_response)

        # Send data to microcontroller through serial monitor
        ser.write(api_response.encode('utf-8'))

    except Exception as e:
        print(f"API call failed: {e}")

    time.sleep(API_CALL_INTERVAL)  # Enforce rate limit

def get_latest_distance():
    # Function to get the latest distance reading from the serial port
    last_line = ''
    while ser.in_waiting > 0:
        line = ser.readline().decode('utf-8').rstrip()
        if line.isdigit():  # Only consider valid distance readings
            last_line = line
    return last_line

# Set up video capture
video = cv2.VideoCapture(0)  # Initialize the webcam

# Get reference images from the user
ref_image1_path = input("Upload an image of the first pet: ")
ref_image1_base64 = encode_image_to_base64(ref_image1_path)
ref_image2_path = input("Upload an image of the second pet: ")
ref_image2_base64 = encode_image_to_base64(ref_image2_path)

# Main loop variables
frame_counter = 0
skip_initial_frames = 5  # Frames to skip at the beginning
processing_thread = None  # Thread for processing images

while True:
    success, frame = video.read()  # Read a frame from the webcam
    if not success:
        break

    frame_counter += 1

    # Process every 60th frame after skipping initial frames
    if frame_counter > skip_initial_frames and frame_counter % 60 == 0:
        distance_reading = get_latest_distance()  # Get the latest distance reading
        if distance_reading:
            distance = int(distance_reading)
            if distance <= 30:
                print(f"Distance: {distance} cm")
                # Process the image and call the API only if distance is <= 30 cm
                current_image_base64 = base64.b64encode(cv2.imencode(".jpg", frame)[1]).decode("utf-8")
                if processing_thread is None or not processing_thread.is_alive():
                    processing_thread = threading.Thread(target=call_openai_api, args=(ref_image1_base64, ref_image2_base64, current_image_base64))
                    processing_thread.start()
            else:
                print(f"Distance is greater than 30 cm: {distance} cm")

    cv2.imshow('Frame', frame)  # Display the frame
    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

video.release()  # Release the webcam
cv2.destroyAllWindows()  # Close all OpenCV windows
if processing_thread is not None:
    processing_thread.join()  # Wait for the processing thread to finish

ser.close()  # Close the serial connection
