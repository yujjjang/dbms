Author : Jae Sun Yu (yujjjang3@gmail.com)

Features
=========

*  This database is disk based b+ tree that all records are stored in leaf node.
*  The structure of B+ tree is the most common structure.
*  Only a single process (only a single thread) can access a database at time.
*  Replacement policy is a simple clock replacement algorithm.
*  The basic operations are `find(table_id, key)`, `insert(int table_id, key, value)`, `delete(table_id, key)`.


Using A Database
==============

This database management system has totally seven operations. The following example shows how to use a this system. It is pretty easy to use.



1.   `int init_db (int buf_num)`  <pre><code>#include "include/bpt.h"
 if (init_db(100) != 0) {
   printf("Fail to initialize DB\n");
   exit(-1);
} </code></pre>
This function is used to initialize buffer pool with given number and buffer manager. Other functions should be called after init_db() is called.
1.  `int open_table (char* pathname)`  <pre><code>#include "include/bpt.h"
int table_id = open_table("sample.txt"); </code></pre>
This function is used to open existing data file using 'pathname' or create one if not existed. If success, it returns its unique table id.
1.  `int insert (int table_id, int64_t key, char* value)` <pre><code>#include "include/bpt.h"
if (insert(table_id, key, value) != 0) {
printf("Cannot insert the key\n");
exit(-1);
}</code></pre>
This function is used to insert key and value in table. If success, it return 0.

1.  `char * find (int table_id, int64_t key)` <pre><code> #include "include/bpt.h"
if (find(table_id, key) == NULL) {
printf("Cannot find the value using by given key\n");
exit(-1);
}</code></pre>
This function is used to find the value using given key. If success, it returns the value.

1.  `int delete (int table_id, int64_t key)` <pre><code> #include "include/bpt.h"
if (delete(table_id, key) != 0) {
printf("Cannot delete the record using by given key\n");
exit(-1);
}</code></pre>
This function is used to delete the record including the given key. If success, it returns 0.

1.  `int close_table (int table_id)` <pre><code> #include "include/bpt.h"
if (close_table(table_id) != 0) {
printf("Fail to close the table\n");
exit(-1);
}</code></pre>
This function is used to close the table whose id is a given table id. Before closing, It writes the pages relating to this table to disk. If success, it returns 0.

1.  `int shutdown_db (void)` <pre><code> #include "include/bpt.h"
if (shutdown_db() != 0) {
printf("Fail to shut down db\n");
exit(-1);
} </code></pre>
This function is used to shut down database management system. Before doing, It writes the pages in buffer to disk. If success, it returns 0.



Performance
========
I used two test cases. The first is arbitrary number and the second is the continuous number. The process of testing is as follows. First, insert the given 1,000,000 number into this database. Then delete it in the same order as the given number. I did calculate the amount of time to insert and to delete according to the buffer size. Calculated time is the average of the time obtained from 10 cases.

# Test Results




Analysis
===========
Before doing, I'm not sure that my program is running in right way. There is no problem with correctness but maybe problem with efficiency. So analysis will based on the assumption my program works well. Thank you.
*  In cases that there is no buffer, we could see the difference in time due to how much the given data is sorted.

*  If the cache ratio is less than half, approximately, buffer manager does not work. Rather it leads to poor performance.

*  As you can see in above data graph, increasing the number of buffers does not improve the efficiency. I don't know the reason now. I am going to get the reason as soon as possible.

*  The clock replacement algorithm is not working well in random aligned data but pretty fine in adequately data. 













