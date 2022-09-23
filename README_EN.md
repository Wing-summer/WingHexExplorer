如果你是中文使用者，请转到 [这里](README.md)

- [QHexView](#qhexview)
  - [Features](#features)
  - [Buffer Backends](#buffer-backends)
  - [License](#license-1)
    - [About QHexEdit2](#about-qhexedit2)
  - [Screenshot](#screenshot)
  - [Donation](#donation)
  - [Thanks](#thanks)
  - [Repositories](#repositories)
  - [WIKI](#wiki)
  - [Maintaining](#maintaining)
  - [Plugins](#plugins)
  - [App Store](#app-store)

---

<h1 align="center"> WingSummer.WingHexExplorer</h1>

<p align="center">
<img alt="PEHexExplorer" src="WingHexExplorer/images/icon.png">
<p align="center">WingHexExplorer</p>
</p>

<p align="center">
<img alt="Author" src="authorband.svg">
<img alt="License" src="licenseband.svg">
</p>

- It's not so easy to maintain open-source project. Please give me a Star or [donation](#donation) .

## Credit

Before that, I would like to thank everyone for their support and help. The following are people who participated in the contribution and gave donation:

|Nake name|By|Comment|
|:--:|:--:|:--:|
|神末shenmo|donation|Deepin forum|
|lv36|donation|Deepin forum|

## Contributors

One tree does not make a forest.Working together is more important. The following are the people who have contributed to the repository. Thanks for your help:

|Nake name|Contribution|
|:--:|:--:|
|神末shenmo|fix the display bug on ubuntu|

## WingHexExplorer

This software is a hexadecimal editor based on QT, which is developed with C++, so as to make Deepin have a powerful and free hexadecimal editor. At present, only `010 Editor` has powerful hexadecimal editing function, but it is commercially available.If you pay attention to my development trends, you should know that I have developed the `WingSummer.WingCloudHexExplorer` with C# on Windows, which designed to facilitate professionals to modify and analyze PE files, and can be used as an important auxiliary tool for learning PE structures.This project has 31 Stars and 9 Forks, and I don't plan to maintain them, because my main PC operating system is not Windows, and I don't have sufficient financial support. It's all my blood and wishful thinking. No one has participated in any form of contribution to the repository, which may be the status quo of individual open source in China.

As for the requirements of this project, I hope you'd better contribute code or give reference examples so that I can improve quickly. Otherwise, if you propose a seemingly simple function, it actually requires a lot of code to implement, which may be the contradiction between "product manager" and "programmer". When making suggestions, please don't go overboard. Note that this software only provides the most basic hexadecimal editing and browsing services, such as templates and scripts in 010 Editor, which should be implemented through plugins! I hope you will not only raise your needs, but also put forward constructive suggestions and solutions to jointly maintain the open source community. Details will be introduced later.

### Architecture

- BadTestPlugin :Plugins that test the preemption mechanism of the plugin system.
- TestPlugin : Although the name literally means test plugin, it is a very important tutorial for writing plugins supported by the software, and its importance cannot be ignored.
- WingHexExplorer : The main program has the most basic ability to edit hexadecimal, supports a powerful plugin system, and provides GUI interaction.
- qBreakpad : This software captures the library of global exceptions. When an unhandled exception is triggered, a dmp file will be generated.
- QHexView :The basic component of the hexadecimal editor of this software, the maintainer `Dax89`, will be introduced later.
- QHexEdit2 : This software opens the code related to the super large file module. It was originally intended to use this component as the basic component, but I can't change it due to the number of bugs. Therefore, it is discarded and the key code I need is retained. The maintainer is `Simsys`. Details will be introduced later.

### Statement

1. The purpose of developing this software is to enable Deepin to have a powerful and free hexadecimal editor, and it is also convenient for me to consolidate or learn the relevant programming knowledge of QT Linux.
2. This software is only for learning and communication, and shall not be used for commercial purposes without permission. If you want to use some parts of this software for commercial purposes, you must ask me to purchase a commercial license and talk about the price privately.
3. As a student, the software is written in my spare time, I cannot fix bugs or provide technical support in time. Please forgive me.
4. I am not a computer major, and it is inevitable that there will be bugs in my programming. Please submit PR.

### Participation

1. If you want to participate in the software code development and submission, please contact me at pull request.
2. If you are willing to support the project, please go to our repository to make donation through wechat or Alipay. The price of a bottle of water is enough to increase my enthusiasm for maintaining the project. Thank you for your support.
3. If you want to submit the code to repair or improve the program, please submit it in pull request.
4. Anyone who successfully participated in the code bug repair and improvement will be recorded in the ReadMe and attached instruction documents of the repository. If you are one of them, I can explain according to your reasonable wishes.
  

**Joining us does not mean code maintenance. You can choose one or more of the following to participate:**

1. Code maintenance: realize new functions or repair BUGs, and maintain and upgrade codes.
2. Document editing: It is important to write and edit interface documents and tutorials.
3. Participation in discussion: mainly discussing the future development and direction of the project.
4. Write plugins: enhance the functions of the software together.

### License

If the software is an open source version, it will under the `AGPL-3.0`, and should not be used for purposes other than the agreement. I only want to enrich the kinds of softwares on Linux and let the motherland promote the localization of the operating system as soon as possible. Anyone who takes the interests for granted is not welcomed. I don't want to waste my time focusing on copyright issues. I am new to open source license, and I am not likely to choose the right license according to my real needs. Thank BLumia for providing me with relevant suggestions and guidance.

If you want to use the code of this software for closed-source commercial code, and want to remove the `GPL` restriction , you must consult me personally to discuss matters about commercial licensing.

### Release

&emsp;&emsp;To facilitate the management of various packages,all packages are on the  [Lansuyun](https://wwu.lanzoul.com/b021fkd5c) ,the password is `ewtv`。Click to view the only `deb` package that is the latest version.If you want a older version,[Lansuyun](https://wwu.lanzoul.com/b021ogyfi),the password is `8bwy`.Only the packages with the latest version number will be retained, and the rest will be deleted. **I strongly recommend updating with my distribution. Each update will have bug fixes or enhancements.**

### ReadeMe Before an issue

If you have any suggestions, please read the following statement before submitting the issue to avoid wasting our valuable time:

1. I do not consider compatibility with Windows. Although QT is cross-platform, some codes depend on specific platforms, and hexadecimal editing components will have some known problems on Windows. I have no time to do cross-platform things.
2. I don't consider multilingual support, mainly because I don't have time and money. Since I am Chinese, I do not consider other language users. But if you use other languages, simply replace the file if you have a language pack.
3. I don't consider the distribution of the deb binary installation package, and I don't have any time to do anything else.
4. I do not consider the development of other plugins. If there are problems with the use of plugins, please contact the developer (it is not excluded that I will develop some plugins, so you can contact me directly).
5. I don't consider the UI. I use the DTK native style to develop software. If you think it ugly, write a style file and load it.
6. The icon may be a little fit in the dark mode, which is not obvious. I'm not good at art. I hope some people can provide me with a set of icons (no icons with copyright disputes), which will be reviewed and adopted by PR. But if you don't, don't disturb me.
7. I only consider the compatibility with Deepin. Although Deepin is currently based on Debian, almost all Debian and even other Linux may use it, but I do not rule out problems. If there are problems, I will not repair them.

If you are a like-minded open-source contributor, welcome to fork my repository for corresponding maintenance!

If there is a serious bug, I may not respond in time.

### About QHexView

&emsp;&emsp;This software is developed on the basis of `QHexView` as a hexadecimal editor. I add new functions and conduct in-depth code customization on the basis of modified components. The following is a necessary description of the original warehouse. For details, click the [link](https://github.com/Dax89/QHexView/tree/master) ：

---

QHexView
========

QHexView is a hexadecimal widget for Qt5

Features
-----

- Customizable data backend (see more below).
- Document/View based.
- Unlimited Undo/Redo.
- Fully Customizable.
- Fast rendering.
- Easy to Use.

Buffer Backends
-----

These are the available buffer backends:

- QMemoryBuffer: A simple, flat memory array.
- QMemoryRefBuffer: QHexView just display the referenced data, editing is disabled.

It's also possible to create new data backends from scratch.

License
-----

QHexEdit is released under MIT license

---

My improvements on the modified components are as follows:

1. Add functions related to describing file status: indicating whether it is modified, whether it has writable permission, whether it locks the file to prevent modification, and whether it can add or change bytes
2. Add the function of hiding the address column, ASCII decoding character column and header
3. Realize the reading and writing function module of super large files (over 2GB) (the original component does not realize this function)
4. Modify the address display length to adapt to the usage habits of the address
5. Simplify coding mode and delete some redundant codes
6. Add more signals to fully control the QHexView component
7. Add bookmark usage and management functions
8. Fix the bug that the relevant contents of the scroll bar can still be displayed but still scroll
9. Fix the problem that the flashing position of the pasting pointer remains unchanged, and modify the pasting restriction strategy
10. Add some interfaces to meet the needs of workspace support
11. Fix the bug with excessive part in the first line of the background color of the mark
12. Fix the bug that the bytes in the copy contain zero bytes and will be truncated

### About QHexEdit2

At first, I planned to use `QHexEdit2` as the hexadecimal editor for development. Although this component can easily open large files, its editing function can be used, but there are many bugs, large and small. I also repaired them one by one, but I found that only my strength and time are a drop in the bucket. Then I found the `QHexView`, which is the component above, but it has a fatal flaw. It can't open large files and was lost by my Pass. Later, I tried to use it. I found that developers have made enough efforts in developing and modifying components, and editing is very smooth. Recently, I saw that the contributors of `QHexView` wanted to build a `QHexView 5.0`. They refactored the code, but they did not implement any functions. It was almost an empty framework. However, from the interface, we can see that it was more powerful and easy to use. This is something that the original component does not have. It took me more time to read the source code and extend the interface outward to meet my development needs.

Then I thought that since `QHexEdit2` has a powerful ability to open files, while `QHexView` does not, but it has a powerful editing interface, I transplanted the code of `QHexEdit2` to open super large files into `QHexView`, and made adaptation and function enhancement. Link to original warehouse:< https://github.com/Simsys/qhexedit2 >Its agreement is as follows:

Copyright (C) 2015-2016 Winfried Simon

This software may be used under the terms of the GNU Lesser General
Public License version 2.1 as published by the Free Software Foundation
and appearing in the file license.txt included in the packaging of this file.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

## Screenshot

<p align="center">
<img alt="Screenshot" src="screenshot.png">
<p align="center">WingHexExplorer</p>
</p>

## Donation

**<p align="center">Every piece of your support will be a powerful driving force for the promotion of this project. Thank you very much for your support</p>**

<p align="center">

<img alt="Alipay" src="WingHexExplorer/支付宝捐助.jpg" height=50% width=50%>
<p align="center">Thanks for donation</p>

</p>

<p align="center">
<img alt="Wechat" src="WingHexExplorer/微信捐助.png" height=50% width=50%>
<p align="center">Thanks for donation</p>

</p>

## Thanks

The completion of this procedure is basically that I learn and use. Although I have learned QT before, I have hardly written a complete project. DTK is also new to me, but in general, the documentation is still a little poor and incomplete. However, the official source code of the text editor has been opened. I have studied the code I am interested in and need to implement, and realized the functions I want. Without this project, I cannot complete this project alone. In some places, there are some similarities, because this is the code that I learned and modified, such as the implementation of single instance and parameter transfer, the use of DTK settings dialog box, internationalization, and the style and code structure of jump bars.

## Repositories

&emsp;&emsp;Recently, I found a warehouse, which is very simple and allows you to download the distribution without logging in. It is enough for me to maintain the software warehouse. Gitee is only used for backup in the future, but it can also be submitted to issue or PR. If you want to contribute ideas or code suggestions to GitLink ： https://www.gitlink.org.cn/wingsummer/WingHexExplorer 。

If you want to visit Gitea, please go to：https://code.gitlink.org.cn/wingsummer/WingHexExplorer .In fact, it is GitLink, but it seems that GitLink shows a bug in the repository image. **Recommended here!**

If you want to visit Gitee: https://gitee.com/wing-cloud/wing-hex-explorer .Welcome to my new warehouse to star or feedback bugs and contribute code.

If you want to visit Github :  https://github.com/Wing-summer/WingHexExplorer .GitHub is only used for backup, but considering foreign friends, it retains the issue and PR. Forgiving me taking little care of Github repositories because of poor network access to it.

## WIKI

If you want to learn how to use the WingHexEditor and how to develop plugins for this software, please go to this link: https://code.gitlink.org.cn/wingsummer/WingHexExplorer/wiki/%E7%AE%80%E4%BB%8B .At the same time, we welcome you to point out the mistakes of Wiki and contribute high-quality content.

## Maintaining

&emsp;&emsp;In the future, I will not be so diligent in maintenance. I plan to release every month if there is an update. If there is no update, it will be postponed. However, bug fixing must be earlier than the release. If you have the ability, please read my wiki to compile and replace it. If I provide more donations, I will put more effort into it.

## Plugins

- WingElfParser：A Marker plugin for Elf.[Gitea](https://code.gitlink.org.cn/wingsummer/WingElfParser) | [Gitee](https://gitee.com/wing-cloud/wing-elf-parser)
- WingHexPy：A useful plugin for python3.7 support.[Gitea](https://code.gitlink.org.cn/wingsummer/WingHexPy) | [Gitee](https://gitee.com/wing-cloud/wing-hex-py)
- WingHexDisasm：A small disassembly plugin for WingHexExplorer.[Gitea](https://code.gitlink.org.cn/wingsummer/WingHexDisasm) | [Gitee](https://gitee.com/wing-cloud/wing-hex-disasm)
- WingHexAsm：A small assembly plugin for WingHexExplorer.[Gitea](https://code.gitlink.org.cn/wingsummer/WingHexAsm) | [Gitee](https://gitee.com/wing-cloud/wing-hex-asm)

## App Store

&emsp;&emsp;The software can be installed through the app store. Currently, it supports Deepin store and Spark store only in Chinese.
