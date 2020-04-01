// Program: fluidAnalysis.scala
//
import org.apache.spark.sql.SparkSession
import org.apache.spark.sql.functions._
import org.apache.spark.sql.types._
import com.redislabs.provider.redis._

import org.apache.spark.sql.streaming.Trigger

object FluidAnalysis {
    def main(args: Array[String]): Unit = {
         val spark = SparkSession
                     .builder()
                     .appName("fluid-analysis")
                     .master("local[*]")
                     .config("spark.redis.host", "localhost")
                     .config("spark.redis.port", "6379")
                     .getOrCreate()

         val fluids = spark
                     .readStream
                     .format("redis")
                     .option("stream.keys","fluids")
                     .schema(StructType(Array(
                           StructField("step", LongType),
                           StructField("region_id", LongType),
                           StructField("valuelist", StringType),
                      )))
                      .load()
//          val bystep = fluids.groupBy("step").count
          val region0 = fluids.select("step", "valuelist").where("region_id = 1")
          
          /*
          val fluidWriter : fluidForeachWriter =
new fluidForeachWriter("localhost","6379")
          
          val query = bystep
                      .writeStream
                      .outputMode("update")
                      .foreach(fluidWriter)
                      .start()
                      */
          
	     val query = region0
            .writeStream
            .outputMode("update")
            .format("console")
            .trigger(Trigger.ProcessingTime("10 seconds"))
            .start()

          query.awaitTermination()

     } // End main
} //End object
