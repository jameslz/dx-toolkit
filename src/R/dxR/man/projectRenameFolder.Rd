\name{projectRenameFolder}
\alias{projectRenameFolder}
\title{projectRenameFolder API wrapper}
\usage{
  projectRenameFolder(objectID,
    inputParams = emptyNamedList, jsonifyData = TRUE,
    alwaysRetry = FALSE)
}
\arguments{
  \item{objectID}{DNAnexus object ID}

  \item{inputParams}{Either an R object that will be
  converted into JSON using \code{RJSONIO::toJSON} to be
  used as the input to the API call.  If providing the JSON
  string directly, you must set \code{jsonifyData} to
  \code{FALSE}.}

  \item{jsonifyData}{Whether to call \code{RJSONIO::toJSON}
  on \code{inputParams} to create the JSON string or pass
  through the value of \code{inputParams} directly.
  (Default is \code{TRUE}.)}

  \item{alwaysRetry}{Whether to always retry even when no
  response is received from the API server}
}
\value{
  If the API call is successful, the parsed JSON of the API
  server response is returned (using
  \code{RJSONIO::fromJSON}).
}
\description{
  This function makes an API call to the
  \code{/project-xxxx/renameFolder} API method; it is a
  simple wrapper around the \code{\link{dxHTTPRequest}}
  function which makes POST HTTP requests to the API
  server.
}
\seealso{
  \code{\link{dxHTTPRequest}}
}

