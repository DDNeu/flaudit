
## Test summary
![image](https://user-images.githubusercontent.com/67744347/191753483-5bf7a14b-99e2-463b-8aa0-18b822a1786a.png)

#### [TL/DR] Test summary
go to [test results](#lustre-io-performance-impact-summary)
 
## Test details
- Test type: **[specstorage SWBUILD (customized)](#io-type-percentages)**
- Number of runs: **792**
- Total runtime: **66 hours**

#### Test case summary
Test case | Description
--- | --- 
changelog-only | dummy changelog consumer to avoid filling MDT	
flaudit-nopath-10 | flaudit without Fid2Path buffer size 10 records	
flaudit-nopath-100 | flaudit without Fid2Path buffer size 100 records	
flaudit-nopath-1000 | flaudit without Fid2Path buffer size 1000 records	
flaudit-10 | flaudit buffer size 10 records	
flaudit-100 | flaudit buffer size 100 records	
flaudit-1000 | flaudit buffer size 1000 records	

## Test results

#### Lustre IO performance impact summary
Case overhead | IOPS % | Latency % | Read throughput % | Write throughput %
--- | :---: | :---: | :---: | :---:
 **no-changelog** | 0,0% | 0,0% | 0,0% | 0,0%
 **changelog-only** | -7,4% | 8,5% | -7,4% | -7,3%
 **flaudit-nopath (AVG)** | -7,5% | 8,6% | -7,2% | -7,3%
 **flaudit (AVG)** | -7,5% | 8,6% | -7,2% | -7,3%

#### Lustre IO performance impact details
![image](https://user-images.githubusercontent.com/67744347/191757100-8b534843-cf3d-4459-96c3-bfa93646892b.png)

#### Changelog consumption summary
![image](https://user-images.githubusercontent.com/67744347/191759236-b32a7056-3c17-44c6-b7b7-fa992b3b9972.png)

![image](https://user-images.githubusercontent.com/67744347/191759330-689195c6-7d4c-4a6d-bd6e-f6976f10a8ed.png)

## Specstorage parameters

#### Run details
- Instances: 1
- File_size: 16k
- Dir_count: 10
- Files_per_dir: 50

#### IO type percentages

IO Type | %
--- | --- 	
Read_percent	 | 	0
Read_file_percent	 | 	5
Mmap_read_percent	 | 	0
Random_read_percent	 | 	0
Write_percent	 | 	0
Write_file_percent	 | 	5
Mmap_write_percent	 | 	0
Random_write_percent	 | 	0
Read_modify_write_percent	 | 	5
Mkdir_percent	 | 	5
Rmdir_percent	 | 	5
Unlink_percent	 | 	5
Unlink2_percent	 | 	5
Create_percent	 | 	5
Append_percent	 | 	5
Lock_percent	 | 	5
Access_percent	 | 	5
Stat_percent	 | 	5
Neg_stat_percent	 | 	5
Chmod_percent	 | 	5
Readdir_percent	 | 	5
Copyfile_percent	 | 	5
Rename_percent	 | 	5
Statfs_percent	 | 	5
Pathconf_percent	 | 	5
Trunc_percent	 | 	5
Custom1_percent	 | 	0
Custom2_percent	 | 	0


Homepage
---------------
back to [Homepage](https://github.com/DDNeu/flaudit)
