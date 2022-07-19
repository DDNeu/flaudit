flaudit
-------

`flaudit` reads Lustre Changelogs using liblustreapi and write output in json. Based on [lauditd](https://github.com/stanford-rc/lauditd)


Installation
------------

#### Binary:
a binary package is already available in flaudit directory with all relevant files.
copy flaudit directory in `/opt/ddn/` 
```
$ mkdir -p /opt/ddn/
$ cp -a flaudit /opt/ddn/
```
and skip to [running flaudit](https://github.com/DDNeu/flaudit#running-flaudit)

#### Source:

Building binary:

```
$ ./autogen.sh
$ ./configure
$ make
```
copy flaudit directory in `/opt/ddn/` and copy newly compiled `flaudit` binary from `src/flaudit/` to `/opt/ddn/flaudit/`

```
$ mkdir -p /opt/ddn/
$ cp -a flaudit /opt/ddn/
$ cp src/flaudit/flaudit /opt/ddn/flaudit/flaudit
```

Output format
-------------

`flaudit` can be used in conjunction with [fluent-bit](https://fluentbit.io/) to send data through its [output plugins](https://docs.fluentbit.io/manual/pipeline/outputs).
this solution is suitable to send out data to several collection tools like [elasticsearch](https://www.elastic.co/)


Running flaudit
---------------

`flaudit` doesn't need any configuration file as it takes all its parameters from
command line. One flaudit process should be used per Lustre MDT. You will need
to set up a Changelog reader ID dedicated to `flaudit`.
Keep in mind changelog performance impact, see [Official Lustre Manual Changelog section](https://doc.lustre.org/lustre_manual.xhtml#lustre_changelogs)

In this example lustre filesystem `exafs` has only one mdt `MDT0000`.

#### On the MDS:

###### create a changelog mask suitable for your needs
```
lctl set_param mdd.exafs-MDT0000.changelog_mask="CREAT MKDIR HLINK SLINK MKNOD UNLNK RMDIR RENME RNMTO CLOSE LYOUT TRUNC SATTR XATTR HSM MTIME CTIME MIGRT FLRW RESYNC"
```

###### register a changelog user 
```
$ lctl --device exafs-MDT0000 changelog_register audit
exafs-MDT0000: Registered changelog userid 'cl3-audit'
```

#### Use `flaudit` On a lustre client:

`flaudit` should be run on a Lustre client with the filesystem you want to audit
already mounted (read-only is supported). 

###### Run `flaudit` alone (using standard output)

```
$ /opt/ddn/flaudit/flaudit -u cl3-audit exafs-MDT0000
```

Run `flaudit` daemon `flauditd` with fluent-bit and elasticsearch
-----------------------------------------------

requires fluent-bit and a working elastic stack. In this example lustre client has all the software installed locally:

- fluent-bit
- elasticsearch listen on localhost with ssl (default installation)
- kibana listening on 0.0.0.0:443

## ElasticSearch configuration

- an index `lustre-changelog-exafs` is created
- a simple `fluentbit` user is created, with `all` privileges for index `lustre-changelog-*`

## fluent-bit configuration

you can use fluent-bit provided configuration file adjusting relevant output section:

```
[INPUT]
    name  stdin

[OUTPUT]
    Name es
    Match *
    Host localhost
    Port 9200
    tls on
    tls.verify off 
    HTTP_User fluentbit
    HTTP_Passwd *****
    Id_Key id
    Suppress_Type_Name On
    Index lustre-changelog-exafs
```

NOTE:
`host`,`port`,`tls`,`tls.verify` are relevant for default elasticsearch installation. `Suppress_Type_Name` must be set to `on` for current elasticsearch version.

## launch flauditd
you can use the provided wrapper script `flauditd` 

```
flauditd cl3-audit exafs-MDT0000
```

For Linux systems using systemd, you can edit the service unit file provided and copy to `/etc/sysconfig/flauditd`

```
$ cp /opt/ddn/flaudit/flauditd.service /etc/systemd/system/flauditd.service
$ systemctl daemon-reload
```
To start `flauditd`, use:

```
$ systemctl start flauditd
```

To enable `flauditd` at boot time, use:

```
$ systemctl enable flauditd
```
