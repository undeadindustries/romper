# Romper

Organize MAME roms into profiles that you actually want to play.

## Description

There are over 7000 games in a MAME set. Most people only want to play a few.  
Most of their filenames seem random so finding just the games you want isn't straight forward.  
Romper can help you create smaller sets of MAME libraries.  
You could also simply use Romper to search for the name of a MAME file to see which game it is for.  
I don't know about you, but I don't need any screenless games.  
Possibly you are trying to make an SD Card for your RetroPie and don't want ALL of MAME.  

Ideas of sets you can create from your own library or online libraries:
* Every Capcom or other specific publisher's games.
* Just to top ranked games.
* Only games in the shooter genre.

You may filter by rank, genre and screenless. 
You may search by name, description, publisher and more.

Romper uses data from [MAME data sets](https://pleasuredome.github.io/pleasuredome/mame/) and organized into one DB.

Currently, it only runs in 64bit Linux.
Other 64bit platforms are currently planned: Linux Arm, Windows (x86-64), Mac OS 

I would love others to help contribute code fixes, clean up and efficiencies. I'm here to learn.


## Background

Romper was originally built in WxGo(I really like Go) to organize not only MAME, but every console romset as well.
I toyed with using Flutter instead of Wx. But Flutter just wasn't there yet.
QT wasn't used because their licensing worried me.
It was less useful for consoles because those files have sane names. So I limited Romper's scope to MAME (non-merged) sets. 
Many DB schemas and types were tested.
In the end, the simplest DB worked the best. 
The DB is created from MAME XML and INI files.
There is a Golang app to create the DB. I'm not sure if this will be put on Github.
I've been a developer for decades. But this is my first C++ app. (Be nice when reporting issues) 
The m_ standard naming for variables is not used because almost everything is a member.


## Getting Started / Downloading

Download the latest release: https://github.com/undeadindustries/romper/releases
If you are in Linux, installed Fuse2. https://github.com/AppImage/AppImageKit/wiki/FUSE
There should me instructions on the release page on how to run the current version.  
You will be prompted to create your first profile.  
![](https://user-images.githubusercontent.com/9536461/221396822-ca925ab1-c7a6-4003-b0a7-3f8157fc10c4.png)  
  
Let's create a best-of profile.  
I named the profile. I checked "Download files automatically" because I don't have the MAME files.  
I then selected the target download folders for the MAME files.  
Then Save.  
![](https://user-images.githubusercontent.com/9536461/221396825-05fe92b1-e8fa-4918-a5ad-2cfc74c34b53.png)  
  
Once saved, you'll see a list of all MAME files, descriptions etc.  
Let's narrow the results by clicking the SELECT menu, de-selecting SCREENLESS,  
and de-selecting all RANKS other than the best games (80-100).  
![](https://user-images.githubusercontent.com/9536461/221396827-072bb024-16b0-4975-91d8-b8a4010d819a.png)  
  
By design, the SELECT menu doesn't update the search results. Click the SEARCH button and you'll see your narrowed results.  
![](https://user-images.githubusercontent.com/9536461/221396828-5e699c98-949c-4e52-91a9-4b2886d9de68.png)  
  
Now select which games you'd like to download and click RUN to start downloading. That's about it!

## Considerations

* Downloading happens from Archive.org. It's not fast. It took me 13 hours to download "Best Games" only.  
* I might have it look to see if you've already downloaded some files and not auto-overwrite.  
* If you plan on making large profile sets, Download the [Non-Merged MAME ROM and CHD](https://pleasuredome.github.io/pleasuredome/mame/) files first.  
* I stress that this is only tested with Non-Merged sets.  
* Most games don't need CHD files. CHD files are large. Google it for more info.  
* The files work. If you are getting an error, update your MAME software and make sure it is pointing to the correct CHD folder.  
* One day I may support images, videos, marquees, etc. But not yet.  

## Dependencies

* 64bit amd/intel. Arm support soon
* CPP17
* WxWidgets
* SQLiteCpp
* AppImage for linux packaging

## Compiling

* Soon, I'll create a sample .vscode folder.
* For Linux: If you download and compile WxWidgets yourself. ../configure --enable-debug --with-opengl --with-gtk=3 --disable-shared --enable-webrequest && make && make install 
* Windows, MacOS, and RaspberryPi Arm coming soon.

## Help

I'll try to fix any issues you report.
* Please be cool and understanding. This project is likely at the bottom of my priority list.
* I am interested in help maintaining this. But unlikely to add any or many new features.
* I would love help maintaining it.

## Version History

* 2025-02-23
    * Rewrote the CMake and really the whole compile process.
    * Fixed a few bugs during the search.
    
* 2023-04-14
    * Converted from VS Code Tasks to CMake.
    * Using LinuxDistro AppImageTool to package.
    * README.MD updates.
* 2023-02-27
    * The first Linux 64 candidate.
    * README.MD.
* 2022-02-09
    * Cleaned up some of the code
    * Hitting enter in search box now searches
* 2022-02-05
    * Initial Release

## License

This project is licensed under the LGPL License because that was closest to WxWidgets' license - see the LICENSE.md file for details

## Acknowledgments
forums.wxwidgets.com answered SO MANY of my questions. Thank you!