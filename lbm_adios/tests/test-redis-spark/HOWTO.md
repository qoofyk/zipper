See https://www.infoq.com/articles/data-processing-redis-spark-streaming/

## prerequisite:
1. see  installation in root README.


#### Steps for ubuntu 1804 vm instances
0. prepare redis 

  ```
  sudo apt-get update
  sudo apt-get install redis
  ```

  change /etc/redis/redis.conf.

  1. comment out bind localhost
  2. set protection mode = no
  3. set supervised systemd

  ```
  sudo systemctl restart redis
  ```

1. prepare sbt 
  sbt not available in ubuntu official repo, try wget https://dl.bintray.com/sbt/debian/sbt-1.3.4.deb)
  ```
  wget https://dl.bintray.com/sbt/debian/sbt-1.3.4.deb
  sudo dpkg -i sbt-1.3.4.deb
  sudo apt-get update
  sudo
  ```

2. build using sbt
  ```
  sbt package
  ```

3. install pysparks
  ```
  conda create --name pyspark pyspark
  conda activate pyspark
  ```

4. start he deamon (this will keep checking update and do some "analysis"), write analysis results into 
  ```
  run_analysis.sh
  ```

5. in a terminal 
```
src/redis_client and then insert values(how data is ingested)
```



## other methods

#### self-built redis

1. start redis_server(https://cloud.google.com/community/tutorials/setting-up-redis)
```
src/redis server ../redis.conf
```

#### run with docker

or  run a server in backgroud, map to 1993
```
sudo docker run --name myredis -d -p 6379:1993 redis:5
```

get the ip of this redis:
```
sudo  docker inspect --format='{{range .NetworkSettings.Networks}}{{.IPAddress}}{{end}}' myredis
 172.17.0.2
```

start a redis cli too
```
sudo docker run --name myredis-cli -it --rm redis:5 redis-cli -h 172.17.0.2
```


#### check analysis results using spark-sql(not added yet)

```
run_query.sh
```

then in the sql interface:
```
CREATE TABLE IF NOT EXISTS atoms(step STRING, count INT) USING org.apache.spark.sql.redis OPTIONS (table 'atom');
select * from atoms;
```
```
CREATE TABLE IF NOT EXISTS clicks(asset STRING, count INT) USING org.apache.spark.sql.redis OPTIONS (table 'click');
select * from clicks;
```


