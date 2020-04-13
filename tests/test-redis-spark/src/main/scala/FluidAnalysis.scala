// Program: fluidAnalysis.scala
//
import java.io.File
import org.apache.spark.sql.SparkSession
import org.apache.spark.sql.functions._
import org.apache.spark.sql.types._
import com.redislabs.provider.redis._

import org.apache.spark.sql.streaming.Trigger
import org.apache.spark.sql.Row
import org.apache.spark.sql.Dataset

import org.apache.spark.rdd.RDD
import org.apache.spark.SparkFiles

object FluidAnalysis {
		def getListOfFiles(dir: String):List[File] = {
					val d = new File(dir)
					if (d.exists && d.isDirectory) {
							d.listFiles.filter(_.isFile).toList
					} else {
							List[File]()
					}
}
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
                           StructField("valuelist", StringType)
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
          
          /*
	     val query_fake = region0
            .writeStream
            .outputMode("update")
            .format("console")
            .trigger(Trigger.ProcessingTime("10 seconds"))
            .start()

          query_fake.awaitTermination()
          */
          val filelist = getListOfFiles("./")
          println("The empty list is: " + filelist) 

          // val scriptPath = SparkFiles.get("compute_dmd.py")
          //val py_command="env python " + SparkFiles.get("run_fluiddmd.py")
          val py_command="env python3 ./run_fluiddmd.py" // + SparkFiles.get("run_fluiddmd.py")
          val query_py = region0
            .writeStream
            .outputMode("update")
            .format("console")
            .trigger(Trigger.ProcessingTime("10 seconds"))
            .foreachBatch { (batchDF: Dataset[Row], batchId: Long) =>
               // Transform and write batchDF 
               val rows: RDD[Row] = batchDF.rdd
               //val pipeRDD = rows.pipe("env python compute_dmd.py")
               val pipeRDD = rows.pipe(py_command)
               pipeRDD.collect().foreach(println)
            }.start()

          query_py.awaitTermination()

     } // End main
} //End object
