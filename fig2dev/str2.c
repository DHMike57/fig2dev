char *strstr(s1, s2)
    char *s1, *s2;
{
    int len2;
    char *stmp;

    len2 = strlen(s2);
    for (stmp = s1; *stmp != NULL; stmp++)
	if (strncmp(stmp, s2, len2)==0)
	    return stmp;
    return NULL;
}
