paleofetch
==========

Like neofetch, but incomplete, written in ~200 lines of C instead of 10,000 lines of bash,
and runs in like 0.005 seconds.

This is a WIP, and definitely not a minimum viable product. Currently it only supports Arch Linux with X running. 

If anyone has input for the features I have yet to implement, I would love to hear them.

Example output:

![example output](example.png)

Compiling
---------
```bash
mkdir build
cd build

cmake ..
make
```

FAQ
---

Q: Do you really run neofetch every time you open a terminal?  
A: Yes, I like the way it looks and like that it causes my prompt to start midway
down the screen. I do acknowledge that the information it presents is not actually useful.
