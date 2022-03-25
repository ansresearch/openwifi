# About CSIobfuscator branch

<img src="./images/ans-logo-4x.png" width="900">

Developed by the [ANS research group](https://ans.unibs.it) at the [University of Brescia](https://www.unibs.it/en), Italy

<img src="./images/unibs-logo-4x.png" width="900">

## Short description

We call *obfuscation* the act of hiding non-communication-related
information, distinguishing it from the more common *jamming*,
whose goal is simply destroying the entire communication
capability of the system.

We implemented a CSI fuzzer so to obfuscate those properties of the CSI
that an advanced attacker could use, without authorization, to localize a user in an indoor environment.

In [this fork](https://github.com/ansresearch/openwifi/tree/CSIobfuscator) of the [openwifi project](https://github.com/open-sdr/openwifi) we implemented the driver side of our CSI fuzzer.


## Main modifications compared to the original project

The introduced modifications to the openwifi driver are the following:

- Addition of [DebugFs](https://www.kernel.org/doc/html/latest/filesystems/debugfs.html) resources
to enable/disable obfuscation and to monitor the obfuscation process
- Generation of obfuscation random processes (Supported by this [Fixed-Point Algebra Library](https://sourceforge.net/projects/fixedptc) by [Ivan Voras](https://sourceforge.net/u/ivoras/profile))
- Transfer of obfuscation random coefficients to the FPGA 


## Enable obfuscation features in the driver

The user can follow the original [guide for updating driver](https://github.com/open-sdr/openwifi#Update-Driver)
so to compile the driver source code that includes our obfuscation features.

**NB:** Don't forget to: 

- Install on your development board the FPGA image that supports obfuscation; available [here](https://github.com/ansresearch/openwifi-hw)
- Copy the driver (*sdr.ko*) on the desired development board


## Control obfuscation via the driver

When the user loads the driver:

1. Obfuscation is initially turned OFF (i.e. DISABLED)
2. Two DebugFs resources, namely, *obstatus* and *ansfile*, are created under the common *ansdbg/* folder

        root@analog:~/openwifi>ls /sys/kernel/debug/ansdbg/
        ansfile  obstatus

**Obfuscation status**

*obstatus* supports [read&write operations](https://www.kernel.org/doc/htmldocs/filesystems/API-debugfs-create-u32.html)

When the content of *obstatus* is a number larger than 0, then obfuscation is turned ON (i.e., ENABLED).
Otherwise, obfuscation is turned OFF (DISABLED)

Examples of controlling/monitoring obfuscation from a terminal:

        root@analog:~/openwifi>cat /sys/kernel/debug/ansdbg/obstatus 
        0
        root@analog:~/openwifi>echo 1 > /sys/kernel/debug/ansdbg/obstatus 
        root@analog:~/openwifi>dmesg
        User turned ON obfuscation
        root@analog:~/openwifi>cat /sys/kernel/debug/ansdbg/obstatus 
        1
        root@analog:~/openwifi>
        root@analog:~/openwifi>
        root@analog:~/openwifi>echo 0 > /sys/kernel/debug/ansdbg/obstatus 
        root@analog:~/openwifi>
        root@analog:~/openwifi>dmesg
        User turned ON obfuscation
        User turned OFF obfuscation
        ANS auxiliary vars CLEANED (obfuscation is OFF)
        [0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 ]
        root@analog:~/openwifi>cat /sys/kernel/debug/ansdbg/obstatus 
        0
        root@analog:~/openwifi>


**Monitoring Obfuscation Process**

The driver is fundamentally responsible of updating a Markovian random process that steers the distortion
of transmitted signals and thus achieve obfuscation. Distortion coefficients are defined for each subcarrier, this is why
*ansfile* is a vector of *Nsc* values, where *Nsc*=64 in the present case reflecting the fact that so far openwifi supports OFDM
over 20MHz channels.

When the obfuscation is OFF the obfuscation coefficients should all be zeros, when it is instead ON, then coefficients are
updated at each packet transmission and the user could monitor the evolution of the random process making many consecutive reads
of the *ansfile*

Examples of monitoring the obfuscation random process

* Turn OFF obfuscation and check coefficients

        root@analog:~/openwifi>echo 0 > /sys/kernel/debug/ansdbg/obstatus 
        root@analog:~/openwifi>cat /sys/kernel/debug/ansdbg/ansfile 
        0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0

* Turn ON obfuscation and continuously check coefficients

        root@analog:~/openwifi>echo 1 > /sys/kernel/debug/ansdbg/obstatus 
        root@analog:~/openwifi>cat /sys/kernel/debug/ansdbg/ansfile 
        0.7589,1.1389,1.1589,1.00399,0.91631,0.89262,0.67873,0.70981,0.63689,0.78548,0.89175,0.87156,0.92462,1.16713,0.94581,0.92855,1.07741,1.16254,1.08413,1.04553,0.84065,0.68396,0.73936,0.81563,0.98617,0.96619,1.02331,0.91186,0.88282,0.81341,0.79444,0.74385,0.93635,0.99995,1.20331,1.20034,1.35198,1.2017,0.9568,0.80636,0.94567,0.66939,0.51395,0.7927,0.86548,0.86078,0.99635,1.15782,0.88297,0.84959,0.89099,0.75542,0.95396,1.00631,1.0096,0.98404,1.01699,0.99522,0.93323,0.8732,0.91341,1.0880,0.72977,0.63426
        root@analog:~/openwifi>
        root@analog:~/openwifi>
        root@analog:~/openwifi>
        root@analog:~/openwifi>cat /sys/kernel/debug/ansdbg/ansfile 
        0.61299,0.63298,0.65298,0.5633,0.32318,0.62421,0.79349,0.9587,1.04023,1.19234,1.25131,1.43732,1.31178,1.11218,0.92471,0.57201,0.40085,0.5498,0.67117,0.79817,0.79087,0.96675,1.13813,1.21654,1.35852,1.40262,1.18456,0.91514,0.83193,0.68408,0.71731,0.88584,0.79526,0.86034,1.02944,1.03661,0.97282,1.16312,1.01572,0.98084,1.13521,1.04909,0.86524,1.1910,1.16799,1.09125,0.9972,0.99075,0.81355,0.60364,0.5631,0.50111,0.84374,0.92506,1.01936,1.07809,1.25453,1.08644,1.01724,0.83074,0.90229,1.04824,0.8537,0.63877
        root@analog:~/openwifi>
        root@analog:~/openwifi>
        root@analog:~/openwifi>
        root@analog:~/openwifi>cat /sys/kernel/debug/ansdbg/ansfile 
        0.35044,0.37044,0.62804,0.46197,0.54881,0.8500,1.20867,1.2145,1.45011,1.63964,1.4844,1.48574,1.32441,1.19027,1.04125,0.8953,0.56902,0.48691,0.33102,0.44079,0.76375,0.92144,1.05425,1.02212,1.10891,0.78595,0.79013,0.75304,0.89002,0.78593,0.93384,1.02605,0.93033,1.11551,1.0900,0.98054,1.01131,1.23638,0.87638,0.95166,1.27322,1.0769,1.02196,1.38196,1.35646,1.18054,1.1781,1.00796,0.74483,0.68555,0.73765,0.70548,1.06548,1.22195,1.11059,1.2344,1.50678,1.45703,1.35394,1.28049,1.23066,1.1416,0.81135,0.6411

