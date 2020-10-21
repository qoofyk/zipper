// Program: fluidAnalysis.scala
//
import java.io.File
import java.time.Instant
import java.time.Duration
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
                     .getOrCreate()

        val num_regions = args(0).toInt
        var dry_run = 0
        if(args.size == 2 && args(1).toInt == 1){
          dry_run = 1
        }
      
        val region_ids = (0 to (num_regions -1))

        // "region0,region1, region2"
        // https://github.com/RedisLabs/spark-redis/blob/master/doc/structured-streaming.md
        val stream_keys = region_ids.mkString("region",",region","")
         

         val fluids = spark
                     .readStream
                     .format("redis")
                     .option("stream.keys", stream_keys)
                     .schema(StructType(Array(
                           StructField("step", LongType),
                           StructField("localid", LongType),
                           StructField("valuelist", StringType)
                      )))
                      .load()
          
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
          println("[DEBUG]: The empty list is: " + filelist)

          if(dry_run == 1){
            println("-- Streaming processing started, DRYRUN with no dmd")
            val query_py = fluids.groupBy("localid").count()
                .writeStream
                .outputMode("complete")
                .format("console")
                .foreachBatch { (batchDF: Dataset[Row], batchId: Long) =>
                   println("--- batchid," + batchId)

                   val t_start = Instant.now()
                   println(" |- batch" + batchId + ",start-time=" + Instant.now())
                   batchDF.show()
                   val t_finish = Instant.now()
                   println(" |- batch" + batchId + ",finish-time=" + t_finish)
                   println(" |- batch" + batchId + ",elapsed-time(ms)=" +  Duration.between(t_start, t_finish).toMillis() )
                   println("")
                   // batchDF.unpersist()
                }.start()
              query_py.awaitTermination()
          }
          else{
            // val scriptPath = SparkFiles.get("compute_dmd.py")
            //val scriptPath = SparkFiles.get("run_fluiddmd.py")
            // val scriptPath = SparkFiles.get("wc.py")
            val scriptPath = "./run_fluiddmd.py"
            println("-- Streaming processing started: using script in:" + scriptPath)
            val py_command="env python3 " + scriptPath
            // val py_command="cat"
            // val py_command="env python3 ./run_fluiddmd.py" // + SparkFiles.get("run_fluiddmd.py")

            val query_py = fluids.select("step","valuelist")
                .writeStream
                .outputMode("update")
                .format("console")
                .trigger(Trigger.ProcessingTime("3 seconds"))
                .foreachBatch { (batchDF: Dataset[Row], batchId: Long) =>
                   // batchDF.persist() // otherwise each each batchDF will only update one region
                   // println("--- batchid," + batchId+ ", nr_rows:" + batchDF.count())
                   println("--- batchid," + batchId)
                   // Transform and write batchDF 

                   //regions.zipWithIndex.par.foreach{ case (region, id) =>
                   val t_start = Instant.now()
                   println(" |- batch" + batchId + ",start-time=" + Instant.now())
                     // batchDF.show()
                   val pipeRDD = batchDF.rdd.pipe(py_command)
                   pipeRDD.collect().foreach(line => println("---one region:" + line))

                   val t_finish = Instant.now()
                   println(" |- batch" + batchId + ",finish-time=" + t_finish)
                   println(" |- batch" + batchId + ",elapsed-time(ms)=" +  Duration.between(t_start, t_finish).toMillis() )
                   println("")
                   // batchDF.unpersist()
                }.start()
              query_py.awaitTermination()
          }

     } // End main
} //End object
