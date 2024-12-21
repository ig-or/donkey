
import cv2
import pyttsx4
from ultralytics import YOLO
import math
#import torch
import time
import sys
import select
import tty
import termios
import threading
import queue


exit_flag = False  # Global flag to stop the loop


def check_stdin():
    if select.select([sys.stdin,],[],[],0.0)[0]:
        return sys.stdin.read(1)
    return None


def set_input_mode():
    """Set the terminal to non-canonical mode."""
    fd = sys.stdin.fileno()
    old_settings = termios.tcgetattr(fd)
    try:
        new_settings = old_settings[:]
        new_settings[3] &= ~termios.ICANON  # Disable canonical mode (line buffering)
        new_settings[3] &= ~termios.ECHO    # Disable echo (characters typed are not shown)
        termios.tcsetattr(fd, termios.TCSADRAIN, new_settings)
    except Exception as e:
        print(f"Error setting terminal input mode: {e}")
    return old_settings

def reset_input_mode(old_settings):
    """Restore the terminal to its original settings."""
    termios.tcsetattr(sys.stdin.fileno(), termios.TCSADRAIN, old_settings)

def check_keypress():
    """Check if a key has been pressed."""
    dr, dw, de = select.select([sys.stdin], [], [], 0.0)
    if dr:
        char = sys.stdin.read(1)  # Read one character
        return char
    return None

# Create a thread-safe queue for TTS commands
tts_queue = queue.Queue()
def tts_worker():
    engine = pyttsx4.init()  # Object creation
    engine.connect('error', onSoundWrror)
    # Get the current rate
    current_rate = engine.getProperty('rate')
    current_vol = engine.getProperty('volume')
    print(f"Current speech rate: {current_rate}; volume: {current_vol}")
    engine.setProperty('rate', int(current_rate * 0.8)) 
    engine.setProperty('volume', current_vol * 0.16)

    while True:
        command = tts_queue.get()
        if command is None:  # Stop the thread if None is received
            break
        engine.say(command)
        engine.runAndWait()

    engine.say("Buy!")
    engine.runAndWait()
    engine.stop()


model = YOLO("yolo11n.engine")

def is_key_pressed():
    """Check if a key has been pressed (non-blocking)"""
    dr, dw, de = select.select([sys.stdin], [], [], 0)
    if dr:
        ch = sys.stdin.read(1)
        print("ch = " + str(ch))
        return ch
    return None
def onSoundWrror(name, exception):
    print(f"sound Error: {exception}")

#if __name__ == "__main__":

def main():
    global exit_flag
    print("starting. Press ESC to exit...")

    # create the pipeline
    gp = (
        "nvarguscamerasrc ! "   # sensor-id=0   num-buffers=166 
        "video/x-raw(memory:NVMM), width=1640, height=1232, framerate=30/1  ! "
        "nvvidconv  flip-method=0 ! "
        "videorate ! video/x-raw, width=1280, height=720, framerate=10/1 ! "
        "videoconvert ! "
        "video/x-raw, format=BGR ! appsink"
    )
    #print("torch.cuda.is_available()" +  str(torch.cuda.is_available()))
    #gp = "nvarguscamerasrc num-buffers=166 ! nvvidconv ! video/x-raw, width=1280, height=720 ! videoconvert ! video/x-raw, format=BGR ! appsink"
    cap = cv2.VideoCapture(gp, cv2.CAP_GSTREAMER)
    if not cap.isOpened():
        print("Failed to open the camera")
        return
    print("**************camera was opened*****************")
    counter = 0
    iCounter = 0
    # Start the TTS worker thread
    tts_thread = threading.Thread(target=tts_worker, daemon=True)
    tts_thread.start()
    tts_queue.put("Hello!")

    tbCounter = 0
    sHello = True

    seeTB = False
    haveTB = False

    while not exit_flag:
        key = check_keypress()  #
        if key is not None:
            print("Character entered: " +  str(key) + " " + str(ord(key)))
            print(f"Key pressed: '{key}' (ASCII: {ord(key)})")
            if key == '\r':  # If Enter is pressed, handle it normally
                print("Carriage return (\\r) detected automatically by terminal.")
            elif ord(key) == 27:  # ESC key
                exit_flag = True
                print("ESC key pressed, exiting.")
                break
            
        counter += 1
        #if counter % 10 == 0:
        #    print(f"image {counter} iCounter = {iCounter}")
        time.sleep(0.005)
        #if counter > 500:
        #    break

        ret_val, img = cap.read()
        if not ret_val:
            continue
        iCounter += 1
        results = model(img, stream=True, verbose=False)
        detections = list(results)  # Convert to list
        n = len(detections)

        haveTB = False
        for result in detections:
            for a in result:
                pr = a.boxes.conf[0].item()
                if pr < 0.65:
                    continue                
                id = int(a.boxes.cls[0].item())
                name = a.names[id]
                if id == 77:
                    haveTB = True

        if haveTB:
            if tbCounter < 5:
                tbCounter += 1
        else:
            if tbCounter > 0:
                tbCounter -= 1

        if seeTB:
            if tbCounter == 0:
                seeTB = False
                tts_queue.put("where is my teddy bear?")

        else:
            if tbCounter == 5:
                seeTB = True
                tts_queue.put("Hello teddy bear!")

        if iCounter % 15 == 0:
            print(f"n = {n}")
            nr = 0
            for result in detections:
                nr += 1
                na = 0
                for a in result:
                    na += 1
                    if sHello:
                        print(f"names: {a.names}");
                        sHello = False
                    pr = a.boxes.conf[0].item()
                    if pr < 0.65:
                        continue
                    id = int(a.boxes.cls[0].item())
                    name = a.names[id]
                    w = int(a.boxes.xywh[0][2].item());
                    h = int(a.boxes.xywh[0][3].item());
                    print(f"id = {id}; name: {name} (%.2f); {w}x{h}" % pr)
                    #print(f"nr = {nr}, na = {na}")

                    #print("PROBS:")
                    #print(a.probs)
                    #print("BOXES:")
                    #print(a.boxes)
                    #print(int(a.boxes.cls[0].item()))
                    #print(a.boxes.conf[0].item())
            print(" ---- ")

    # Signal the thread to exit
    tts_queue.put(None)  # Add a None to stop the worker
    tts_thread.join()
    cap.release()
    cv2.destroyAllWindows()


old_settings = set_input_mode()  # Set terminal to non-blocking mode
try:
    main()
finally:
    reset_input_mode(old_settings) 


