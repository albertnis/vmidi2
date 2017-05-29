# VMIDI2

*Capturing piano performance with depth vision*

![Example of tracked difference image][thumb]

This program uses OpenCV 3.2.0 and the depth stream from a Kinect V2 camera to track keypress events on a piano keyboard. Crucially, the information captured includes the velocity of each keypress.

## Notes

In its current state, VMIDI2 solely performs keypress detection. Keyboard registration, MIDI output and automatic music transcription are not implemented.

The perspective transform points and keyboard overlay in the code are manual to suit my recordings, so they will have to be tweaked if using different footage. I am more than happy to provide anyone with the main recordings (.xef files) I used, so get in touch if you are interested. They're many gigabytes in size so I decided not to host them.

### Visual Studio Setup

VMIDI2 was developed in Visual Studio 2017 and has also been tested on the 2015 edition. The key thing is to make sure OpenCV is referenced correctly. Ensure OpenCV and the Kinect SDK are properly installed, as detailed in the *Installation* section.

If using the provided .sIn file, just be sure to set configuration to Release/x64 and you should be good to go.

Else if working from the .cpp source, the Visual Studio solution can be set up as follows:

1. Right click the project in Solution Explorer and open properties.
1. In the drop-down menus at the top of the window, select "All Configurations" and "All Platforms".
1. Edit "VC++ Directories/Include Directories" to add *$(OPENCV_DIR)\\..\\..\include*.
1. Edit "Linker/General/Additional Library Directories" to add *$(OPENCV_DIR)\lib* and *$(KINECTSDK20_DIR)\Lib\x64*.
1. Edit "Linker/Input/Additonal Dependencies" to add *opencv_world320.lib* and *Kinect20.lib*.


### Running the Code

1. Open Kinect recording file (.xef) in Kinect Studio.
1. Press the connection button while in the *PLAY* tab. This connects the recording file to the computer as if it were a physical Kinect device.
1. Set inpoints and outpoints as desired within the recording file using the timeline. Be sure that there are no hands or pressed keys present at the start of playback.
1. Ensure playback is stopped.
1. Run VMIDI2 application. The command line will show some matrices. After these are shown, the program is ready to receive playback.
1. Press the play button in Kinect Studio. Many OpenCV windows will show. Keypresses will be logged to the command line in the format {keyname},{velocity}.

## Installation (Windows)

### Kinect Studio

1. Get the Kinect for Windows SDK 2.0 from [Microsoft][kinect].
1. Use the included installer.
1. (The installer should do this automatically) Double check that there is a system environment variable named *KINECTSDK20_DIR* with a value pointing to the Kinect SDK (e.g. *C:\Program Files\Microsoft SDKs\Kinect\v2.0_1409\*).

### OpenCV

1. Get pre-built OpenCV 3.2.0 from [SourceForge][cv]. 
1. Install to a convenient location such as C:\opencv
1. Edit system environment variables:
    1. Create a variable named *OPENCV_DIR* with a path to the OpenCV build (e.g. *C:\opencv\build\x64\vc14*).
    1. Add to the *PATH* variable the OpenCV binary directory (e.g. *C:\opencv\build\x64\vc14\bin*).

## Useful Links

- Check out [my blog post][blog].
- Get the [research paper][paper].

[blog]: https://albertnis.com/posts/piano-vision/
[paper]: https://albertnis.com/resources/2017-05-10-piano-vision/Nisbet_Green_Capture_of_Dynamic_Piano%20Performance_with_Depth_Vision.pdf
[cv]: https://sourceforge.net/projects/opencvlibrary/files/opencv-win/3.2.0/
[kinect]: https://www.microsoft.com/en-nz/download/details.aspx?id=44561
[thumb]: http://i.imgur.com/ZXZFZBe.png