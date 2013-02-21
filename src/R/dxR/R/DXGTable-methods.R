# Copyright (C) 2013 DNAnexus, Inc.
#
# This file is part of dx-toolkit (DNAnexus platform client libraries).
#
#   Licensed under the Apache License, Version 2.0 (the "License"); you may not
#   use this file except in compliance with the License. You may obtain a copy
#   of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
#   WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
#   License for the specific language governing permissions and limitations
#   under the License.

##' @import methods

library(methods)

##' GenomicTable handler constructor
##'
##' Construct a GenomicTable(GTable) handler using an object ID of the
##' form "gtable-xxxx".  If a project ID is not provided, the default
##' project (project or a job's temporary workspace) is used if available.
##'
##' @param id String object ID
##' @param project String project or container ID
##' @param describe Whether to cache a description of the gtable
##' @return An R object of class DXGTable
##' @rdname DXGTable
##' @examples
##' DXGTable("gtable-123456789012345678901234", describe=FALSE)
##' DXGTable("gtable-123456789012345678901234", project="project-12345678901234567890abcd", describe=FALSE)
##' @export
DXGTable <- function(id, project=dxEnv$DEFAULT_PROJECT, describe=TRUE) {
  handler <- new("DXGTable", id=id, project=project)
  if (describe) {
    desc(handler) <- describe(handler)
  }
  return (handler)
}

# TODO: constructors for opening a gtable for reading or writing

##' Get the ID from a DNAnexus handler
##'
##' Returns the string ID of the gtable.
##' 
##' @param handler An object with a class that has the "id" slot
##' @return string ID of the referenced object
##' @docType methods
##' @rdname id-methods
##' @examples
##' dxgtable <- DXGTable("gtable-123456789012345678901234", describe=FALSE)
##' id(dxgtable)
##' @export
##' @aliases id,DXGTable-method
setMethod("id", "DXGTable", function(handler) {
  handler@id
})

##' Describe the Data Object
##'
##' Returns the data frame containing columns describing the data
##' object.
##' 
##' @param handler A data object handler
##' @return named list of the data object's describe hash
##' @docType methods
##' @rdname describe-methods
##' @export
##' @aliases describe,DXGTable-method
setMethod("describe", "DXGTable", function(handler) {
  validObject(handler)
  inputHash <- RJSONIO::emptyNamedList
  if (handler@project != '') {
    inputHash$project <- handler@project
  }
  dxHTTPRequest(paste("/", handler@id, "/describe", sep=''),
                data=inputHash)
})

setReplaceMethod("desc", "DXGTable", function(object, value) {
  object@desc <- value
  return (object)
})

##' Get Column Names of a GTable
##'
##' Returns a character vector of column names of the GTable.
##' 
##' @param x A GTable handler
##' @return vector of column names
##' @docType methods
##' @rdname names-methods
##' @export
##' @aliases names,DXGTable-method
setMethod("names", "DXGTable", function(x) {
  if (length(x@desc) == 0) {
    desc(x) <- describe(x)
  }

  if ("columns" %in% names(x@desc)) {
    return (sapply(x@desc$columns, function(coldesc) { return (coldesc[["name"]]) }))
  } else {
    stop("Did not find list of columns in the gtable description")
  }
})

setMethod("getRows", "DXGTable", function(handler) {
  gtableGetRows(handler@id)
})
