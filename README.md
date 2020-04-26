# seam-carver
OpenCV based utility for efficient content-aware scaling.
Includes a basic wrapper for user input but the main focus is on the functionality under the hood. Feel free to add any GUI bindings for easy usage.

Basic usage:

```Carver.run <mode: vertical/horizontal/both> <scale factor: 0-1> <input path> <output path>```

#### How does it work?
[Wikipedia](https://en.wikipedia.org/wiki/Seam_carving) includes a decent explanation on the topic. The basic idea is to 
pick a method to assign an importance value to each pixel and then locate the least important seams through the image. This
solution uses gradient magnitude (energy) as the importance value. Energies are updated each time a seam has been cut from 
the image to achieve best possible quality.

Asynchronous C++ threading and  OpenMP are both used for better performance. These features can be disabled by removing the define:

```#define CONCURRENT```


#### Example output when running in vertical mode
![image](https://user-images.githubusercontent.com/36196504/80315324-72c2ca00-87ff-11ea-97c0-80c9b8d8c2aa.jpg)
![image](https://user-images.githubusercontent.com/36196504/80315328-77877e00-87ff-11ea-939c-c01988cc9c9d.jpg)
