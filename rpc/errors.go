package api

import (
	"context"

	"github.com/btmc-code/base/pseudohsm"
	"github.com/btmc-code/base/rpc"
	"github.com/btmc-code/base/signers"
	"github.com/btmc-code/base/txbuilder"
)

var (
	// ErrDefault is default Btminer API Error
	ErrDefault = errors.New("Btminer API Error")
)

func isTemporary(info httperror.Info, err error) bool {
	switch info.ChainCode {
	case "BTMC000": // internal server error
		return true
	case "BTMC001": // request timed out
		return true
	case "BTMC761": // outputs currently reserved
		return true
	case "BTMC706": // 1 or more action errors
		errs := errors.Data(err)["actions"].([]httperror.Response)
		temp := true
		for _, actionErr := range errs {
			temp = temp && isTemporary(actionErr.Info, nil)
		}
		return temp
	default:
		return false
	}
}

var respErrFormatter = map[error]httperror.Info{
	ErrDefault: {500, "BTMCC000", "Btminer API Error"},

	// Signers error namespace (2xx)
	signers.ErrBadQuorum: {400, "BTMC200", "Quorum must be greater than or equal to 1, and must be less than or equal to the length of xpubs"},
	signers.ErrBadXPub:   {400, "BTMC201", "Invalid xpub format"},
	signers.ErrNoXPubs:   {400, "BTMC202", "At least one xpub is required"},
	signers.ErrDupeXPub:  {400, "BTMC203", "Root XPubs cannot contain the same key more than once"},

	// Contract error namespace (3xx)
	ErrCompileContract: {400, "BTMC300", "Compile contract failed"},
	ErrInstContract:    {400, "BTMC301", "Instantiate contract failed"},

	// Transaction error namespace (7xx)
	// Build transaction error namespace (70x ~ 72x)
	account.ErrInsufficient:         {400, "BTMC700", "Funds of account are insufficient"},
	account.ErrImmature:             {400, "BTMC701", "Available funds of account are immature"},
	account.ErrReserved:             {400, "BTMC702", "Available UTXOs of account have been reserved"},
	account.ErrMatchUTXO:            {400, "BTMC703", "Not found UTXO with given hash"},
	ErrBadActionType:                {400, "BTMC704", "Invalid action type"},
	ErrBadAction:                    {400, "BTMC705", "Invalid action object"},
	ErrBadActionConstruction:        {400, "BTMC706", "Invalid action construction"},
	txbuilder.ErrMissingFields:      {400, "BTMC707", "One or more fields are missing"},
	txbuilder.ErrBadAmount:          {400, "BTMC708", "Invalid asset amount"},
	account.ErrFindAccount:          {400, "BTMC709", "Not found account"},
	asset.ErrFindAsset:              {400, "BTMC710", "Not found asset"},
	txbuilder.ErrBadContractArgType: {400, "BTMC711", "Invalid contract argument type"},
	txbuilder.ErrOrphanTx:           {400, "BTMC712", "Not found transaction input utxo"},
	txbuilder.ErrExtTxFee:           {400, "BTMC713", "Transaction fee exceed max limit"},

	// Submit transaction error namespace (73x ~ 79x)
	// Validation error (73x ~ 75x)
	validation.ErrTxVersion:                 {400, "BTMC730", "Invalid transaction version"},
	validation.ErrWrongTransactionSize:      {400, "BTMC731", "Invalid transaction size"},
	validation.ErrBadTimeRange:              {400, "BTMC732", "Invalid transaction time range"},
	validation.ErrNotStandardTx:             {400, "BTMC733", "Not standard transaction"},
	validation.ErrWrongCoinbaseTransaction:  {400, "BTMC734", "Invalid coinbase transaction"},
	validation.ErrWrongCoinbaseAsset:        {400, "BTMC735", "Invalid coinbase assetID"},
	validation.ErrCoinbaseArbitraryOversize: {400, "BTMC736", "Invalid coinbase arbitrary size"},
	validation.ErrEmptyResults:              {400, "BTMC737", "No results in the transaction"},
	validation.ErrMismatchedAssetID:         {400, "BTMC738", "Mismatched assetID"},
	validation.ErrMismatchedPosition:        {400, "BTMC739", "Mismatched value source/dest position"},
	validation.ErrMismatchedReference:       {400, "BTMC740", "Mismatched reference"},
	validation.ErrMismatchedValue:           {400, "BTMC741", "Mismatched value"},
	validation.ErrMissingField:              {400, "BTMC742", "Missing required field"},
	validation.ErrNoSource:                  {400, "BTMC743", "No source for value"},
	validation.ErrOverflow:                  {400, "BTMC744", "Arithmetic overflow/underflow"},
	validation.ErrPosition:                  {400, "BTMC745", "Invalid source or destination position"},
	validation.ErrUnbalanced:                {400, "BTMC746", "Unbalanced asset amount between input and output"},
	validation.ErrOverGasCredit:             {400, "BTMC747", "Gas credit has been spent"},
	validation.ErrGasCalculate:              {400, "BTMC748", "Gas usage calculate got a math error"},

	// VM error (76x ~ 78x)
	vm.ErrAltStackUnderflow:  {400, "BTMC760", "Alt stack underflow"},
	vm.ErrBadValue:           {400, "BTMC761", "Bad value"},
	vm.ErrContext:            {400, "BTMC762", "Wrong context"},
	vm.ErrDataStackUnderflow: {400, "BTMC763", "Data stack underflow"},
	vm.ErrDisallowedOpcode:   {400, "BTMC764", "Disallowed opcode"},
	vm.ErrDivZero:            {400, "BTMC765", "Division by zero"},
	vm.ErrFalseVMResult:      {400, "BTMC766", "False result for executing VM"},
	vm.ErrLongProgram:        {400, "BTMC767", "Program size exceeds max int32"},
	vm.ErrRange:              {400, "BTMC768", "Arithmetic range error"},
	vm.ErrReturn:             {400, "BTMC769", "RETURN executed"},
	vm.ErrRunLimitExceeded:   {400, "BTMC770", "Run limit exceeded because the BTM Fee is insufficient"},
	vm.ErrShortProgram:       {400, "BTMC771", "Unexpected end of program"},
	vm.ErrToken:              {400, "BTMC772", "Unrecognized token"},
	vm.ErrUnexpected:         {400, "BTMC773", "Unexpected error"},
	vm.ErrUnsupportedVM:      {400, "BTMC774", "Unsupported VM because the version of VM is mismatched"},
	vm.ErrVerifyFailed:       {400, "BTMC775", "VERIFY failed"},

	// Mock HSM error namespace (8xx)
	pseudohsm.ErrDuplicateKeyAlias: {400, "BTMC800", "Key Alias already exists"},
	pseudohsm.ErrLoadKey:           {400, "BTMC801", "Key not found or wrong password"},
	pseudohsm.ErrDecrypt:           {400, "BTMC802", "Could not decrypt key with given passphrase"},
}

// Map error values to standard BTMiner error codes. Missing entries
// will map to internalErrInfo.
//
// TODO(jackson): Share one error table across Chain
// products/services so that errors are consistent.
var errorFormatter = httperror.Formatter{
	Default:     httperror.Info{500, "BTMC000", "BTMiner API Error"},
	IsTemporary: isTemporary,
	Errors: map[error]httperror.Info{
		// General error namespace (0xx)
		context.DeadlineExceeded: {408, "BTMC001", "Request timed out"},
		httpjson.ErrBadRequest:   {400, "BTMC002", "Invalid request body"},
		rpc.ErrWrongNetwork:      {502, "BTMC103", "A peer core is operating on a different blockchain network"},

		//accesstoken authz err namespace (86x)
		errNotAuthenticated: {401, "BTMC860", "Request could not be authenticated"},
	},
}
