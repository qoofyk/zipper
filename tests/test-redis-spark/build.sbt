scalaVersion := "2.11.12"
// val sparkRedisVersion ="2.4.2"
val sparkVersion = "2.4.5"

// mainClass in (Compile, run) := Some("fluidAnalysis")

// I don't know how to create two jars in one sbt project..
/*
lazy val clickapp = (project in file("click"))
  .settings(
    name := "ClickAnalysis",
		libraryDependencies ++= Seq(
        "org.apache.spark" %% "spark-core" % sparkVersion,
        "org.apache.spark" %% "spark-sql" % sparkVersion,
        "org.apache.spark" %% "spark-catalyst" % sparkVersion
		)
  )
  */

/*
lazy val atomapp = (project in file("."))
  .settings(
    name := "AtomAnalysis",
		libraryDependencies ++= Seq(
        "org.apache.spark" %% "spark-core" % sparkVersion,
        "org.apache.spark" %% "spark-sql" % sparkVersion,
        "org.apache.spark" %% "spark-catalyst" % sparkVersion,
		)
  )
*/
lazy val fluidapp = (project in file("."))
  .settings(
    name := "fluidAnalysis",
		libraryDependencies ++= Seq(
        "org.apache.spark" %% "spark-core" % sparkVersion,
        "org.apache.spark" %% "spark-sql" % sparkVersion,
        "org.apache.spark" %% "spark-catalyst" % sparkVersion,
        // "com.redislabs" %% "spark-redis" % sparkRedisVersion
    ),
    assemblyJarName in assembly := "fluid-withdeps.jar"
  )
