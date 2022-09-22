
## Test infrastructure


## Test run
- Test type: **[specstorage SWBUILD (customized)](https://github.com/DDNeu/flaudit/new/main#specstorage-swbuild-custom-parameters)**
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



Case overhead | IOPS % | Latency % | Read throughput % | Write throughput %
--- | :---: | :---: | :---: | :---:
 **no-changelog** | 0,0% | 0,0% | 0,0% | 0,0%
 **changelog-only** | -7,4% | 8,5% | -7,4% | -7,3%
 **flaudit-nopath (AVG)** | -7,5% | 8,6% | -7,2% | -7,3%
 **flaudit (AVG)** | -7,5% | 8,6% | -7,2% | -7,3%
 
 
## Specstorage SWBUILD custom parameters

#### Run details
- Instances: 1
- File_size: 16k
- Dir_count: 10
- Files_per_dir: 50

#### Specstorage IO type percentages

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
