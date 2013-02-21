context("DXGTable")

test_that("constructor works", {
  dxgtable <- DXGTable("gtable-12345678901234567890abcd", describe=FALSE)
  expect_that(dxgtable@id, equals("gtable-12345678901234567890abcd"))
})

test_that("id method words", {
  dxgtable <- DXGTable("gtable-12345678901234567890abcd", describe=FALSE)
  expect_that(id(dxgtable), equals("gtable-12345678901234567890abcd"))
})
