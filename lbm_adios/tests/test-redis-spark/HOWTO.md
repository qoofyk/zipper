See https://www.infoq.com/articles/data-processing-redis-spark-streaming/

#### build
1. sbt package

#### Steps
1. start redis_server
```
src/redis server
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
