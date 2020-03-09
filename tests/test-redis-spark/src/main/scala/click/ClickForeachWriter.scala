// Program: ClickForeachWriter.scala
//
package click
import org.apache.spark.sql.ForeachWriter
import org.apache.spark.sql.Row
import redis.clients.jedis.Jedis

class ClickForeachWriter(p_host: String, p_port: String) extends 
ForeachWriter[Row]{

    val host: String = p_host
    val port: String = p_port

    var jedis: Jedis = _

    def connect() = {
        jedis = new Jedis(host, port.toInt)
    }

    override def open(partitionId: Long, version: Long):
 Boolean = {
        return true
    }

    override def process(record: Row) = {
        var asset = record.getString(0);
        var count = record.getLong(1);
        if(jedis == null){
            connect()
        }

        jedis.hset("click:"+asset, "asset", asset)
        jedis.hset("click:"+asset, "count", count.toString)
        jedis.expire("click:"+asset, 300)
    }

    override def close(errorOrNull: Throwable) = {
    }
}
