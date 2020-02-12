// Program: AtomForeachWriter.scala
//
import org.apache.spark.sql.ForeachWriter
import org.apache.spark.sql.Row
import redis.clients.jedis.Jedis

class AtomForeachWriter(p_host: String, p_port: String) extends 
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
        var step = record.getLong(0);
        var count = record.getLong(1);
        if(jedis == null){
            connect()
        }
        println("writing record, step(" + step + "), count(" + count+ ")" )

        jedis.hset("atom:"+step.toString, "step", step.toString)
        jedis.hset("atom:"+step.toString, "count", count.toString)
        jedis.expire("atom:"+step.toString, 300)
    }

    override def close(errorOrNull: Throwable) = {
    }
}
