# Network Flow Clustering

This project is a simple implementation of **network flow clustering** in the C programming language. It uses the **single linkage clustering method** together with a **weighted Euclidean distance** to group similar network communication flows.

The program reads network flow records from an input file, computes distances between flows based on selected features, and repeatedly merges the closest clusters until the desired number of clusters is reached.

## 🔹 How the program works:

* Each network flow initially forms its own cluster.
* The program calculates the distance between flows using selected metrics:

  * total transferred bytes
  * flow duration
  * average interarrival time
  * average packet size
* Cluster distance is determined using the **nearest neighbor principle**.
* The two closest clusters are merged in each iteration.
* The process continues until the required number of clusters remains.
* Final clusters are sorted by the smallest flow ID they contain.

## ✨ Key Features:

* **Loading and validation** of input files.
* **Dynamic memory allocation** for flows and clusters.
* **Weighted Euclidean distance** for comparing network flows.
* **Single linkage clustering** implementation.
* Automatic calculation of:

  * average packet size
  * cluster distances
  * sorted output
* **Cluster sorting** by minimum flow ID.
* Proper error handling for:

  * invalid arguments
  * invalid input file format
  * memory allocation failures

## 🛠 Technologies Used:

* **C11**
* Standard C libraries:

  * `stdio.h`
  * `stdlib.h`
  * `string.h`
  * `math.h`

## ⚙️ Compilation:

```bash
cc -std=c11 -Wall -Wextra -Werror -pedantic flows.c -o flows.exe -lm
```

Or simply:

```bash
make
```

## ▶️ Usage:

Without clustering:

```bash
./flows data.txt
```

With clustering:

```bash
./flows data.txt N WB WT WD WS
```

Example:

```bash
./flows data.txt 2 1.0 1.0 1.0 1.0
```

## 📄 Input File Format:

The first line contains the number of flows:

```text
count=4
```

Each following line contains one network flow:

```text
FLOWID SRC_IP DST_IP TOTAL_BYTES FLOW_DURATION PACKET_COUNT AVG_INTERARRIVAL
```

Example:

```text
count=4
10 192.168.1.1 192.168.1.2 2000 10 20 0.05
11 10.0.0.5 10.0.0.6 4000 20 30 0.07
12 192.168.2.1 192.168.2.3 1500 8 10 0.03
13 172.16.0.10 172.16.0.11 6000 25 45 0.06
```

## 📤 Example Output:

```text
Clusters:
cluster 0: 10 12
cluster 1: 11 13
```

## 🎯 Purpose of the Project:

* Practice working with **structures and dynamic memory**
* Learn how to process structured input files
* Implement a simple **clustering algorithm**
* Understand **weighted Euclidean distance**
* Practice sorting and cluster manipulation in C
* Improve error handling and memory management
