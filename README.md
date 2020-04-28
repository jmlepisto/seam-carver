# seam-carver
#### Work in progress!
OpenCV4 based utility for **efficient multi-threaded** content-aware scaling.
Includes a basic wrapper for user control but the main focus is on the underlying `Carver` class which is used for the actual image processing. 
Feel free to add any GUI bindings or an advanced CLI for easy usage.

Basic usage:

```carver -m <mode> -o <output_path> <input_path>```

#### How does it work?
[Wikipedia](https://en.wikipedia.org/wiki/Seam_carving) includes a decent explanation on the topic. The basic idea is to 
pick a method to assign an importance value to each pixel and then locate the least important seams through the image. This
solution uses gradient magnitude (energy) as the importance value. Energies are updated each time a seam has been cut from 
the image to achieve best possible quality.

Asynchronous C++ processing and threading is used for performance gains. The effect of these really comes to shine when
carving larger images in both directions.

Processing reasonably sized images is quite swift but bear in mind that the actual complexity of this algorithm by image width and height is in the range of

![image](https://latex.codecogs.com/gif.latex?O%28W%24%5Ctimes%24H&plus;W&plus;H%29)

#### How to get it?
**cmake** is the recommended workflow for building this project. The premade cmake definitions include diretives for building both a standalone app as well as a static library.


If you do not feel like building, some premade binaries will be attached to [releases](https://github.com/jjstoo/seam-carver/releases).
Dynamic OpenCV dependencies are:

```
libopencv_imgcodecs.so.4.3
libopencv_imgproc.so.4.3
libopencv_core.so.4.3
```

#### Example output when running in vertical mode
![image](https://user-images.githubusercontent.com/36196504/80315324-72c2ca00-87ff-11ea-97c0-80c9b8d8c2aa.jpg)
![image](https://user-images.githubusercontent.com/36196504/80315328-77877e00-87ff-11ea-939c-c01988cc9c9d.jpg)
