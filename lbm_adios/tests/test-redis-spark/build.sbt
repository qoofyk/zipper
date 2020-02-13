
//scalaVersion := "2.12.10"

val sparkVersion = "2.4.5"
val sparkredisVersion = "2.4.0"
version := "1.0"

/* I don't know how to create two jars in one sbt project..
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

lazy val atomapp = (project in file("."))
  .settings(
    name := "AtomAnalysis",
		libraryDependencies ++= Seq(
        "org.apache.spark" %% "spark-core" % sparkVersion,
        "org.apache.spark" %% "spark-sql" % sparkVersion,
        "org.apache.spark" %% "spark-catalyst" % sparkVersion,
        "com.redislabs" % "spark-redis" % sparkredisVersion
		)
  )
