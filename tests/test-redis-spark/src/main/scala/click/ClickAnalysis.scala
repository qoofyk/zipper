// Program: ClickAnalysis.scala
//
package click
import org.apache.spark.sql.SparkSession
import org.apache.spark.sql.functions._
import org.apache.spark.sql.types._
import com.redislabs.provider.redis._

object ClickAnalysis {
   // def runthis{
    def main_click(args: Array[String]): Unit = {
         val spark = SparkSession
                     .builder()
                     .appName("redis-example")
                     .master("local[*]")
                     .config("spark.redis.host", "localhost")
                     .config("spark.redis.port", "6379")
                     .getOrCreate()

         val clicks = spark
                     .readStream
                     .format("redis")
                     .option("stream.keys","clicks")
                     .schema(StructType(Array(
                           StructField("asset", StringType),
                           StructField("cost", LongType)
                      )))
                      .load()
          val byasset = clicks.groupBy("asset").count
          
          //TODO: write back to redis
          /*
          val clickWriter : ClickForeachWriter =
new ClickForeachWriter("localhost","6379")
          
          val query = byasset
                      .writeStream
                      .outputMode("update")
                      .foreach(clickWriter)
                      .start()
          */
          val query = byasset
            .writeStream
            .outputMode("update")
            .format("console")
            .start()


          query.awaitTermination()

     } // End main
} //End object
