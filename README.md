# OSNDXFIO
<p>The OSNDXFIO module is a set of functions and definitions to setup a low level database (no query language). It is similar to the VMS operating system indexed file I/O functionality.</p>

<p>The OSNDXFIO package opens files without exclusive access and provides no locking/synchronization mechanisms for read/write. Hence, the applications using OSNDXFIO services for database access should use proper synchronization mechanisms. Otherwise, data integrity is not guaranteed.</p>

<p>The OSNDXFIO module creates, rebuilds, opens, closes, and deletes databases. The OSNDXFIO module reads, writes, updates, seeks, and deletes data objects. The OSNDXFIO module is responsible for providing indexing mechanism and defines an index structure that is generic. Applications can define their own index structures, known as the search key in the OSNDXFIO context. There is a practical limit to the number of search keys applied due to performance and memory requirements.</p>

<p>A key descriptor provides information to generate the search key for one single key. The search key could be build from several key segments.</p>
