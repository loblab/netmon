# Real time high precision network monitor

High precision network monitor, for millisecond/microsecond level monitor.
The fastest sampling interval of the program can be 25μs(microsecond) on my i7 PC.
However, the system update period of the network counters seems to be 400μs.
So it is good enough to set the sample cycle to 400μs.

- Platform: Linux (tested on Ubuntu 18/16)
- Ver: 0.1
- Ref: [Technical details (Chinese)](https://www.jianshu.com/p/3dcd14c58cbf)
- Updated: 2/24/2019
- Created: 1/9/2019
- Author: loblab

![Chart1](https://raw.githubusercontent.com/loblab/netmon/master/screenshot/chart1.png)

![Chart2](https://raw.githubusercontent.com/loblab/netmon/master/screenshot/chart2.png)

## Features

- monitor any counters Linux supported. e.g. rx/tx bytes, packets, drop, error
- can set sample cycle/interval & total duration
- support trigger: can set threshold & aggregating function

## Usage

- build.sh: to build the program (from just one source file: netmon.cpp)
- netmon.sh: an example to use the program. modify the options, run it, and you will get a CSV file
- chart.ipynb: a script to draw the chart, run in python notebook with pandas.
  You can use Excel to draw the charts as well.

![Usage](https://raw.githubusercontent.com/loblab/netmon/master/screenshot/usage.png)

