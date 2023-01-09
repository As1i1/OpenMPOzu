# Test log [windows]

## More info and test log [ubuntu] in Github.Actions: [![statusbadge](../../actions/workflows/buildtest.yaml/badge.svg?branch=main&event=pull_request)](../../actions/workflows/buildtest.yaml)

Build log (can be empty):
```
D:\a\itmo-comp-arch22-lab4-As1i1\itmo-comp-arch22-lab4-As1i1\hard.cpp:166:19: warning: implicit conversion from 'int' to 'char' changes value from 170 to -86 [-Wconstant-conversion]
            tmp = 170;
                ~ ^~~
D:\a\itmo-comp-arch22-lab4-As1i1\itmo-comp-arch22-lab4-As1i1\hard.cpp:168:19: warning: implicit conversion from 'int' to 'char' changes value from 255 to -1 [-Wconstant-conversion]
            tmp = 255;
                ~ ^~~
2 warnings generated.

```

Stdout+stderr (./omp4 0 in.pgm out0.pgm):
```
OK [program completed with code 0]
    [STDERR]:  
    [STDOUT]: Time (2 thread(s)): 30.6584 ms
77 130 187 2817.425552

```
     
Stdout+stderr (./omp4 -1 in.pgm out-1.pgm):
```
OK [program completed with code 0]
    [STDERR]:  
    [STDOUT]: Time (-1 thread(s)): 37.5164 ms
77 130 187 2817.425552

```

Input image:

![Input image](test_data/in.png?sanitize=true&raw=true)

Output image 0:

![Output image 0](test_data/out0.pgm.png?sanitize=true&raw=true)

Output image -1:

![Output image -1](test_data/out-1.pgm.png?sanitize=true&raw=true)