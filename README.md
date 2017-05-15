# VMIDI2

*Capturing piano performance with depth vision*

![Example of tracked difference image][thumb]

This program uses OpenCV 3.2.0 and the depth stream from a Kinect V2 camera to track keypress events on a piano keyboard. Crucially, the information captured includes the velocity of each keypress.

## Installation Notes

- Get OpenCV 3.2.0 from [SourceForge][cv].
- Get the Kinect for Windows SDK 2.0 from [Microsoft][kinect].

The perspective transform points and keyboard overlay in the code are manual to suit my recordings, so they will have to be tweaked if using different footage. I am more than happy to provide anyone with the main recordings (.xef files) I used, so get in touch if you are interested. They're many gigabytes in size so I decided not to host them.

## Useful Links

- Check out [my blog post][blog].
- Get the [research paper][paper].

[blog]: https://albertnis.com/posts/piano-vision/
[paper]: https://albertnis.com/resources/2017-05-10-piano-vision/Nisbet_Green_Capture_of_Dynamic_Piano%20Performance_with_Depth_Vision.pdf
[cv]: https://sourceforge.net/projects/opencvlibrary/files/opencv-win/3.2.0/
[kinect]: https://www.microsoft.com/en-nz/download/details.aspx?id=44561
[thumb]: http://i.imgur.com/ZXZFZBe.png