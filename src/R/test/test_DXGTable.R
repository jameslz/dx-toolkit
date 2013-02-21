library(dxR)

context("DXGTable-integration")

idsToDestroy <- vector()

# TODO: make this test even better by replacing the API wrapper usage
# with something better.  Also, column specs.
test_that("describe and names works", {
  inputHash <- list(project=dxR:::dxEnv$DEFAULT_PROJECT, columns=I(list(list(name="foo", type="string"))))
  id <- gtableNew(inputHash)[["id"]]
  idsToDestroy <<- c(idsToDestroy, id)
  handler <- DXGTable(id)
  expect_that(handler@desc[["id"]], equals(id))
  expect_that(length(names(handler)), equals(1))
  expect_that(names(handler)[1], equals("foo"))
})

# teardown

# Ignore any errors
tryCatch({
  dxHTTPRequest(paste("/", dxR:::dxEnv$DEFAULT_PROJECT, "/removeObjects", sep=""), list(objects=I(idsToDestroy)))
})
