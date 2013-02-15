\name{DXGTable}
\alias{DXGTable}
\title{GenomicTable handler constructor}
\usage{
  DXGTable(id, container = dxEnv$DEFAULT_CONTAINER)
}
\arguments{
  \item{id}{String object ID}

  \item{container}{String project or container ID}
}
\value{
  An R object of class DXGTable
}
\description{
  Construct a GenomicTable(GTable) handler using an object
  ID of the form "gtable-xxxx".  If a project ID is not
  provided, the default container (project or a job's
  temporary workspace) is used if available.
}
\examples{
DXGTable("gtable-xxxx")
DXGTable("gtable-xxxx", container="project-xxxx")
}

