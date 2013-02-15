context("DXGTable")

test_that("constructor works", {
  dxgtable <- DXGTable("gtable-xxxx")
  expect_that(dxgtable@id, equals("gtable-xxxx"))
})

test_that("id method words", {
  dxgtable <- DXGTable("gtable-xxxx")
  expect_that(id(dxgtable), equals("gtable-xxxx"))
})
