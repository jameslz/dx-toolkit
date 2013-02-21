\docType{methods}
\name{id}
\alias{id}
\alias{id,DXGTable-method}
\title{Get the ID from a DNAnexus handler}
\arguments{
  \item{handler}{An object with a class that has the "id"
  slot}
}
\value{
  string ID of the referenced object
}
\description{
  Returns the string ID of the gtable.
}
\examples{
dxgtable <- DXGTable("gtable-123456789012345678901234", describe=FALSE)
id(dxgtable)
}

