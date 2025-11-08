# **P2 - Looking for Group Synchronization**

**Task** : Create a solution that will manage the LFG (Looking for Group) dungeon queuing of an MMORPG.

**a)** There are only **n** instances that can be concurrently active. Thus, there can only be a maximum of **n** parties that are currently in a dungeon.  
**b)** A standard party of 5 is composed of **1 tank, 1 healer, and 3 DPS.**  
**c)** The solution should not result in a **deadlock.**  
**d)** The solution should not result in **starvation.**  
**e)** It is assumed that the **input values arrive at the same time.**  
**f)** A time value (in seconds) **t** is randomly selected between **t1** and **t2**, where **t1** represents the fastest clear time of a dungeon instance and **t2** is the slowest clear time. For ease of testing, **t2 <= 15.**
## **User Input**

The program accepts inputs from the user.

* **`n`** - maximum number of concurrent instances

* **`t`** - number of tank players in the queue

* **`h`** - number of healer players in the queue

* **`d`** - number of DPS players in the queue

* **`t1`** - minimum time before an instance is finished

* **`t2`** - maximum time before an instance is finished
  
## **How to run!**

1.  Run the `g++` (or your compiler) command directly:
    ```bash
    g++ main.cpp -o main -std=c++11 -pthread
    ```
2.  Run the executable by typing:
    ```bash
    .\main.exe 
    ```
