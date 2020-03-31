// Program: AtomAnalysis.scala
//
import org.apache.spark.sql.SparkSession
import org.apache.spark.sql.functions._
import org.apache.spark.sql.types._
import com.redislabs.provider.redis._

object AtomAnalysis {
    def atom_main(args: Array[String]): Unit = {
         val spark = SparkSession
                     .builder()
                     .appName("atom-analysis")
                     .master("local[*]")
                     .config("spark.redis.host", "localhost")
                     .config("spark.redis.port", "6379")
                     .getOrCreate()

         val atoms = spark
                     .readStream
                     .format("redis")
                     .option("stream.keys","atoms")
                     .schema(StructType(Array(
                           StructField("step", LongType),
                           StructField("atomid", LongType),
                           StructField("x", FloatType),
                           StructField("y", FloatType),
                           StructField("z", FloatType)
                      )))
                      .load()
//          val bystep = atoms.groupBy("step").count
          val bystep = atoms.groupBy("step").agg(mean("x"), mean("y"), mean("z"), count("atomid"))
          
          /*
          val atomWriter : AtomForeachWriter =
new AtomForeachWriter("localhost","6379")
          
          val query = bystep
                      .writeStream
                      .outputMode("update")
                      .foreach(atomWriter)
                      .start()
                      */
          
	     val query = bystep
            .writeStream
            .outputMode("update")
            .format("console")
            .start()

          query.awaitTermination()

     } // End main
} //End object
