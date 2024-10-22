Flaudit
-------

`flaudit` consumes Lustre Changelogs using liblustreapi and writes output in json. Based on [stanford-rc/lauditd](https://github.com/stanford-rc/lauditd)


Installation
------------

#### Binary:
a binary package is already available in `flaudit` directory with all relevant files.

Copy `flaudit` directory in `/opt/ddn/` 
```
$ mkdir -p /opt/ddn/
$ cp -a flaudit /opt/ddn/
```
and skip to [running flaudit](#running-flaudit)

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
$ cp -a src/flaudit/flaudit /opt/ddn/flaudit/flaudit
```

Output format
-------------

`flaudit` can be used in conjunction with [fluent-bit](https://fluentbit.io/) to send data through its [output plugins](https://docs.fluentbit.io/manual/pipeline/outputs).
this solution is suitable to send out data to several collection tools like [elasticsearch](https://www.elastic.co/)

Running `flaudit`
---------------

`flaudit` doesn't need any configuration file as it takes all its parameters from
command line. One flaudit process should be used per Lustre MDT. You will need
to set up a Changelog reader ID dedicated to `flaudit`.
Keep in mind changelog performance impact, see [Official Lustre Manual Changelog section](https://doc.lustre.org/lustre_manual.xhtml#lustre_changelogs)

In this example lustre filesystem `exafs` has only one mdt `MDT0000`.

#### On the MDS:

create a changelog mask suitable for your needs
```
lctl set_param mdd.exafs-MDT0000.changelog_mask="CREAT MKDIR HLINK SLINK MKNOD UNLNK RMDIR RENME RNMTO CLOSE LYOUT TRUNC SATTR XATTR HSM MTIME CTIME MIGRT FLRW RESYNC"
```

register a changelog user 
```
$ lctl --device exafs-MDT0000 changelog_register audit
exafs-MDT0000: Registered changelog userid 'cl3-audit'
```

#### Use `flaudit` On a lustre client:

`flaudit` should be run on a Lustre client with the filesystem you want to audit
already mounted (read-only is supported). 

Run `flaudit` alone (using standard output)

```
$ /opt/ddn/flaudit/flaudit -u cl3-audit exafs-MDT0000
```

Run `flaudit` daemon `flauditd` with fluent-bit and elasticsearch
-----------------------------------------------

requires fluent-bit and a working elastic stack. In this example all the software is installed locally:

- lustre client with lustreapi, assuming lustre filesystem is mounted (either  `rw` or `ro`)
- fluent-bit ([default RPM installation](https://docs.fluentbit.io/manual/installation/linux/redhat-centos))
- elasticsearch listening on localhost:9200 with ssl ([default RPM installation](https://www.elastic.co/guide/en/elasticsearch/reference/current/rpm.html))
- kibana listening on 0.0.0.0:443 ([default RPM installation](https://www.elastic.co/guide/en/kibana/current/rpm.html), see [this post](https://discuss.elastic.co/t/how-to-use-port-443-to-access-kibana/266757/2) to allow kibana user to bind on port 443)

## Elasticsearch configuration

- index `lustre-changelog-exafs` is created
- `fluentbit` user is created, with `all` privileges for index `lustre-changelog-*`

## Fluent-bit configuration

You can use fluent-bit provided configuration file `/opt/ddn/flaudit/fluent-bit.conf` adjusting relevant output section:

```
[INPUT]
    Name  stdin
    Match *

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

#### NOTE
- `host`,`port`,`tls`,`tls.verify` are relevant for default elasticsearch installation.
- `Suppress_Type_Name` must be set to `on` for elasticsearch version > 8 ([documentation](https://docs.fluentbit.io/manual/pipeline/outputs/elasticsearch#action-metadata-contains-an-unknown-parameter-type)).
- `Id_Key` is elasticsearch Index primary key and [must be provided](https://docs.fluentbit.io/manual/pipeline/outputs/elasticsearch#validation-failed-1-an-id-must-be-provided-if-version-type-or-value-are-set), so is conveniently set as Lustre Changelog ID

## Launch `flauditd`

you can use the provided wrapper script `/opt/ddn/flaudit/flauditd` that uses above configuration file for Fluent-bit.
- NOTE: In /opt/ddn/flaudit/ there is a `fluent-bit` symbolic link pointing the default Fluent-bit installation path (/opt/fluent-bit/)

```
/opt/ddn/flaudit/flauditd cl3-audit exafs-MDT0000
```

## launch `flauditd` via systemd
For Linux systems using systemd, you can edit the provided service unit file `/opt/ddn/flaudit/flauditd.service` and then copy it to `/etc/sysconfig/`

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

Performance
---------------
Go to [Performance page](PERFORMANCE.md) 

Development
---------------
- `make install` and `make rpm` are not yet available :)
- `flaudit` writes its output in a JSON format compatible with `fluent-bit` stdin input plugin. Could be useful to write data also in a more human-readable format and create standard "flat" logfiles as an option.
- In order to send data to Elasticsearch, `flaudit` is tightly coupled with `fluent-bit`. This is not a bad thing per se, but it means that a `flauditd` wrapper daemon instance must be run for each Lustre MDT.
integrating flaudit as a fluent-bit input plugin will allow to use either a single fluent-bit daemon with several `flaudit` input worker threads (one per MDT), or more than one fluent-bit daemon, or a combination of the two.
