# SniffingDetector

## Recording Datasets
### Recorder and Controller

To build with PlatformIO in CLion:
- Clone the project with `git clone https://github.com/Joerg-Schultz/SniffingDetector.git`
- In CLion, File -> Open: SniffingDetector/Recording/RecorderAndController
- In the opening Programming wizard, click 'OK'
- Ignore the error message ;-) and got to Tools -> PlatformIO -> Re-Init
- Edit the Run / Debug configurations: Go to Run -> Edit Configurations...
  Click '+' Button and select 'PlatformIO' Upload and click 'OK'
- Add the CMake environments. Go to File -> Settings -> Build, Execution, Deployment -> CMaker
  Press the '+' Button twice. There should be esp32dev_recorder and esp32dev_controller in the list
  Press OK
- in the 'Edit Run/Debug configurations dialog' you can now select the recorder / the controller and upload it to your ESP32

### App
