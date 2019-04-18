## Cemuhook patches writer

A program that makes writing Cemuhook patches a bit easier by making it more organic and less error-prone.

It basically extends Cemuhook's format with a few Quality of Life features which it then "compiles" into a Cemuhook patch:
* Automatically addresses numbering (no need to refactor code in some cases due to a line change).
* Automatically calculates and inserts code cave size.
* Interprets the loading of constant values in the code cave and makes the addresses for you.
* Makes code cave entries more elegant/smarter.
* Always-on-top mode for easy access.

It doesn't change any existing behavior and using these added features should be fairly easy.

**Latest version for Windows can be downloaded in [Releases](https://github.com/Crementif/CemuhookPatchesWriter/releases).**

#### What does it add?
A good example of all of the different things/behavior can be seen in the [example](https://github.com/Crementif/CemuhookPatchesWriter/blob/master/example/sourcePatches.txt) folder!

```ini
codeCaveSize=auto ; Handles the size on it's own.

codeCaveEntry:
fmuls f3, f3, f2 ; No addresses in front of everything.
lfs f2, .float($aCemuExpression*10)@l(r3) ; Cemu expression directly into a float register, it's possible!
lbz r2, .byte(2) ; Other types are also supported! Also, it will use the same, normal register if you don't hint it using `@l(rX)`.
blr
```

#### How to compile this program on Windows
This assumes you've installed the C++ part in Visual Studio.
1. Download and install [Qt](https://www.qt.io/download). Make sure to select MSVC while it's installing.
2. Clone or download this repository somewhere.
3. Open up the special `Qt X.XX.X (MSVC XXXX XX-bit)` command prompt.
4. Browse to the folder where you've cloned/downloaded the repository and go to the `src` folder.
5. Go to the `src` folder.
6. Find a file called `vcvarsall.bat` somewhere in your Visual Studio installation folder and call it from within the CMD window via the following command (replace the path to the one you've found): `call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" x64`.
7. Run `qmake -config release`
8. Run `nmake`
9. And finally, run `windeployqt --release release`.

You should now have a working `notepad.exe` in the `release` folder!

(I'm honestly not sure if there's an easier way to compile a program from the command prompt, but this worked well enough to create the Release build.)

#### But why

Basically, I wanted to try [Qt](https://www.qt.io/) and had a good idea/need for a tool that would help with some of the aspects of patching ~~and totally not lessen the amount of screw-ups I make~~.

#### License
This project is largely based and build upon Qt's notepad example, which is licensed under [BSD 3-Clause "New" or "Revised" License](https://spdx.org/licenses/BSD-3-Clause.html). This project is also distributed under the same license.
