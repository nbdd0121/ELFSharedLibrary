char* createString() {
	static char str[] = "String@";
	str[6]++;
	return str;
}

void _init() {
	puts("ctor\n");
}

void _fini() {
	puts("dtor\n");
}