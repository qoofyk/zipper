See https://www.infoq.com/articles/data-processing-redis-spark-streaming/

#### prerequisite:
1. see  installation in root README.

#### build
1. download deps
  ```
  sudo apt-get install sbt
  ```

  sbt not available in 16.04, try wget https://dl.bintray.com/sbt/debian/sbt-1.3.4.deb)
  ```
  https://dl.bintray.com/sbt/debian/sbt-1.3.4.deb
  ```

2. sbt package

#### Steps
1. start redis_server
```
src/redis server ../redis.conf
```

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

2. start he deamon (this will keep checking update and do some "analysis"), write analysis results into 
```
run_analysis.sh
```

3. in a terminal 
```
src/redis_client and then insert values(how data is ingested)
```

4. check analysis results using spark-sql
```
run_query.sh
```

5.then in the sql interface:
```
CREATE TABLE IF NOT EXISTS atoms(step STRING, count INT) USING org.apache.spark.sql.redis OPTIONS (table 'atom');
select * from atoms;
```
```
CREATE TABLE IF NOT EXISTS clicks(asset STRING, count INT) USING org.apache.spark.sql.redis OPTIONS (table 'click');
select * from clicks;
```


