# Romper

Organize MAME roms into profiles that you actually want to play.

## Description

There are over 7000 games in a MAME set. Most people only want to play a few.
Romper can help you create smaller sets of MAME libraries. 
You could also simply use Romper to search for the name of a MAME file to see which game it is for.
I don't know about you, but I don't need any screenless games.

Ideas of sets you can create from your own library or online libraries:
* Every Capcom game.
* Just to top ranked games.
* Only games in the shooter genre.

You can filter by rank, genre, screenless and category.

Romper uses data from MAME data sets and organized into one DB.

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
CMAKE is purposely not used. I may be used in the future.
m_ is not used in members because almost everything is a member.


## Getting Started

From the BIN folder, run the romper executable for your platform.

### Dependencies

* CPP17
* WxWidgets
* SQLiteCpp

### Installing

* How/where to download your program
* Any modifications needed to be made to files/folders

## Help

I'll try to fix any issues you report.
* But be cool.
* I am interested in help maintaining this. But unlikely to add any or many new features.

## Version History

* 2022-02-09
    * Cleaned up some of the code
    * Hitting enter in search box now searches
* 2022-02-05
    * Initial Release

## License

This project is licensed under the LGPL License because that was closest to WxWidgets' license - see the LICENSE.md file for details

## Acknowledgments
forums.wxwidgets.com answered SO MANY of my questions. Thank you!