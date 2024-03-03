
/*
 For plugins, store oll their persistent context data in a single structure.
 Allocate it dynamically and associate the pointer in a host app with a slot and the plugin name hash
 In the plugin, hash names of the context struct together. This hash will be reported to a plugin manager and if it changet then the plugin have to be fully reinitialized with a new context
 And plugins should not interface a system directly, only through a host app which should export interface of NPTM::NAPI and some helper functions




*/