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
##' container (project or a job's temporary workspace) is used if available.
##'
##' @param id String object ID
##' @param container String project or container ID
##' @return An R object of class DXGTable
##' @rdname DXGTable
##' @examples
##' DXGTable("gtable-xxxx")
##' DXGTable("gtable-xxxx", container="project-xxxx")
##' @export
DXGTable <- function(id, container=dxEnv$DEFAULT_CONTAINER) {
  new("DXGTable", id=id, container=container)
}

##' Get the ID from a DNAnexus handler
##'
##' Returns the string ID of the gtable.
##' 
##' @param handler An object with a class that has the "id" slot
##' @return string ID of the referenced object
##' @docType methods
##' @rdname id-methods
##' @examples
##' dxgtable <- DXGTable("gtable-xxxx")
##' id(dxgtable)
##' @export
##' @aliases id,DXGTable-method
setMethod("id", "DXGTable", function(handler) {
  handler@id
})
  
