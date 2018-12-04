package go_algorithm


func LegacyAlgorithm(bh, seed *bc.Hash) *bc.Hash {
	cache := calcSeedCache(seed.Bytes())
	data := mulMatrix(bh.Bytes(), cache)
	return hashMatrix(data)
}
