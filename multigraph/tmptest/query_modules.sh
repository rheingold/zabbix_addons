#!/bin/sh
PGPASSWORD='1/Inveks.2' psql -h 192.168.254.16 -U zabbix -d zabbix -c 'SELECT * FROM module;'
