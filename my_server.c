#include <sys/types.h>
#include <microhttpd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/sendfile.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#define PORT            8888
#define POSTBUFFERSIZE  2048
#define MAXNAMESIZE     20
#define MAXANSWERSIZE   2048

#define GET             0
#define POST            1


struct connection_info_struct
{
    int connectiontype;
    char *answerstring;
    struct MHD_PostProcessor *postprocessor;
};

#define DATABASE_PAGE "WebPages/database_page.html"
#define ADDUSER_PAGE "WebPages/add_employee.html"
#define DELETEUSER_PAGE "WebPages/delete_employee.html"
#define TEMP_PAGE "WebPages/temp_page.html"
#define CONTACTUS_PAGE "WebPages/contactus_page.html"
#define EMP_DATA_FILE "employee.dat"
#define TEMPLATE_DATABASE_PAGE "template_pages/template_homepage.html"


const char *errorpage =
    "<html><body><h1>page not found!!!<h1></body></html>";


// employee database structure
typedef struct {
    char tgid[30];
    char name[30];
    char contact[30];
} employee;

employee *buf = NULL;
employee *temp_storage = NULL;
int cnt = 0;

void add_employee(employee**, int*);
void delete_employee(employee**, int *, const char *);
void save_file(employee*, int);
void read_database(employee **, int*);
void write_database_table(employee *, int );
int OSCopyFile(const char*, const char*);
void updateDatabasePage();
void replaceAll(char *, const char *, const char *);


void replaceAll(char *str, const char *oldWord, const char *newWord)
{
    char *pos, temp[2048];
    int index = 0;
    int owlen;

    owlen = strlen(oldWord);
    /*
         * Repeat till all occurrences are replaced.
         */
    while ((pos = strstr(str, oldWord)) != NULL)
    {
        // Bakup current line
        strcpy(temp, str);

        // Index of current found word
        index = pos - str;

        // Terminate str after word found index
        str[index] = '\0';

        // Concatenate str with new word
        strcat(str, newWord);

        // Concatenate str with remaining words after
        // oldword found index.
        strcat(str, temp + index + owlen);
    }
}

void updateDatabasePage()
{
    /* File pointer to hold reference of input file */
    FILE * fPtr;
    FILE * fTemp;
    char path[100] = TEMP_PAGE;
    char buffer[2048];
    char oldWord[20] = "replace_part";


    char * newWord = 0;
    long length;
    FILE * f = fopen ("temp.html", "rb");

    if (f)
    {
        fseek (f, 0, SEEK_END);
        length = ftell (f);
        fseek (f, 0, SEEK_SET);
        newWord = malloc (length);
        if (newWord)
        {
            fread (newWord, 1, length, f);
        }
        fclose (f);
    }

    if (newWord == 0)
    {
        return;
    }


    /*  Open all required files */
    fPtr  = fopen(TEMP_PAGE, "r");
    fTemp = fopen(DATABASE_PAGE, "w");

    /* fopen() return NULL if unable to open file in given mode. */
    if (fPtr == NULL || fTemp == NULL)
    {
        /* Unable to open file hence exit */
        printf("\nUnable to open file.\n");
        printf("Please check whether file exists and you have read/write privilege.\n");
        exit(EXIT_SUCCESS);
    }


    /*
         * Read line from source file and write to destination
         * file after replacing given word.
         */
    while ((fgets(buffer, 2048, fPtr)) != NULL)
    {
        // Replace all occurrence of word from current line
        replaceAll(buffer, oldWord, newWord);

        // After replacing write it to temp file.
        fputs(buffer, fTemp);
    }


    /* Close all files to release resource */
    fclose(fPtr);
    fclose(fTemp);


    /* Delete original source file */
    remove(path);

}

int OSCopyFile(const char* source, const char* destination)
{
    int input, output;
    if ((input = open(source, O_RDONLY)) == -1)
    {
        return -1;
    }
    if ((output = creat(destination, 0660)) == -1)
    {
        close(input);
        return -1;
    }

    //Here we use kernel-space copying for performance reasons
    off_t bytesCopied = 0;
    struct stat fileinfo = {0};
    fstat(input, &fileinfo);
    int result = sendfile(output, input, &bytesCopied, fileinfo.st_size);

    close(input);
    close(output);

    return result;
}

void write_database_table(employee *buf, int cnt)
{
    FILE  *ptrFile = fopen( "temp.html", "w");
    int i;

    fprintf(ptrFile, "\t<tr>\n");
    fprintf(ptrFile, "\t\t<th>NAME</th>\n");
    fprintf(ptrFile, "\t\t<th>TGID</th>\n");
    fprintf(ptrFile, "\t\t<th>CONTACT</th>\n");
    fprintf(ptrFile, "\t</tr>\n");

    for ( i = 0; i < cnt; i++)
    {
        fprintf( ptrFile, "\t<tr>\n");
        fprintf( ptrFile, "\t\t<td>%s</td>\n", (buf + i)->name);
        fprintf( ptrFile, "\t\t<td>%s</td>\n", (buf + i)->tgid);
        fprintf( ptrFile, "\t\t<td>%s</td>\n", (buf + i)->contact);
        fprintf( ptrFile, "\t</tr>");
    }

    fclose( ptrFile );

}

void read_database(employee **bufPtr, int *cntPtr)
{
    ///open the datafile, and build the database-buf
    //find the size, allocate memory
    //copy the file-data into dynamic-block
    //update the actual argument...
    FILE *fp;
    int cnt;
    employee *buf;
    fp = fopen(EMP_DATA_FILE, "r");

    fseek(fp, 0, 2);
    cnt = ftell(fp) / sizeof(employee);

    buf = calloc(cnt, sizeof(employee));
    rewind(fp);
    fread(buf, sizeof(employee), cnt, fp);
    fclose(fp);
    *bufPtr = buf;      // copy to global variable
    *cntPtr = cnt;

}

void save_file(employee *buf, int cnt)
{
    FILE *fp;
    fp = fopen("employee.dat", "w");
    fwrite(buf, sizeof(employee), cnt, fp);
    fclose(fp);
}


void delete_employee(employee **bufPtr, int *cntPtr, const char *id2Delete)
{
    int cnt = *cntPtr;
    employee *temp = *bufPtr;

    while (cnt--)
    {
        if (0 == strcmp(temp->tgid, id2Delete))
            break;
        temp++;
    }

    if (cnt == -1)
    {   printf("record not found\n");
        return;
    }
    else
    {
        printf("record found and deleted successfully\n");
    }

    memmove(temp, temp + 1, cnt * sizeof(employee));
    --(*cntPtr);
    (*bufPtr) = realloc(*bufPtr, (*cntPtr) * sizeof(employee));

}

void print_database(employee *buf, int cnt)
{
    int i;
    for (i = 0; i < cnt; i++)
        printf("tgid=%s name=%s contact=%s\n", (buf + i)->tgid, (buf + i)->name, (buf + i)->contact);
}


void add_employee(employee **bufPtr, int *cntPtr)
{
    (*cntPtr)++;

    (*bufPtr) = realloc(*bufPtr, (*cntPtr) * sizeof(employee));

    strcpy((((*bufPtr) + (*cntPtr) - 1)->name), temp_storage->name);
    strcpy((((*bufPtr) + (*cntPtr) - 1)->tgid), temp_storage->tgid);
    strcpy((((*bufPtr) + (*cntPtr) - 1)->contact), temp_storage->contact);

}


char * getPage(const char* filename)
{
    char * buffer = 0;
    long length;
    FILE * f = fopen (filename, "rb");

    if (f)
    {
        fseek (f, 0, SEEK_END);
        length = ftell (f);
        fseek (f, 0, SEEK_SET);
        buffer = malloc (length);
        if (buffer)
        {
            fread (buffer, 1, length, f);
        }
        fclose (f);
    }

    if (buffer)
    {
        return buffer;
    }
    else
    {
        return "<html><body>Error reading Page</body></html>";
    }


}

static int send_page (struct MHD_Connection *connection, const char *page)
{
    int ret;
    struct MHD_Response *response;


    response =
        MHD_create_response_from_buffer (strlen (page), (void *) page,
                                         MHD_RESPMEM_PERSISTENT);
    if (!response)
        return MHD_NO;

    ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
    MHD_destroy_response (response);

    return ret;
}


static int iterate_post (void *coninfo_cls, enum MHD_ValueKind kind, const char *key,
                         const char *filename, const char *content_type,
                         const char *transfer_encoding, const char *data, uint64_t off,
                         size_t size)
{
    struct connection_info_struct *con_info = coninfo_cls;
    (void)kind;               /* Unused. Silent compiler warning. */
    (void)filename;           /* Unused. Silent compiler warning. */
    (void)content_type;       /* Unused. Silent compiler warning. */
    (void)transfer_encoding;  /* Unused. Silent compiler warning. */
    (void)off;                /* Unused. Silent compiler warning. */

    if (0 == strcmp (key, "home"))
    {
        if ((size > 0) && (size <= MAXNAMESIZE))
        {
            read_database(&buf, &cnt);                          // read binary database file
            write_database_table(buf, cnt);                     // create temp table file
            OSCopyFile(TEMPLATE_DATABASE_PAGE, TEMP_PAGE);      // copy template to temp file, to be mofified
            updateDatabasePage();                               // update database to webpage
            char *answerstring;
            answerstring = malloc (MAXANSWERSIZE);
            if (!answerstring)
                return MHD_NO;

            snprintf (answerstring, MAXANSWERSIZE, getPage(DATABASE_PAGE), data);
            con_info->answerstring = answerstring;
        }
        else
            con_info->answerstring = NULL;

        return MHD_NO;
    }

    if (0 == strcmp (key, "contact_us")) {
        if ((size > 0) && (size <= MAXNAMESIZE))
        {
            char *answerstring;
            answerstring = malloc (MAXANSWERSIZE);
            if (!answerstring)
                return MHD_NO;
            snprintf (answerstring, MAXANSWERSIZE, getPage(CONTACTUS_PAGE), data);
            con_info->answerstring = answerstring;
        }
        else
            con_info->answerstring = NULL;

        return MHD_NO;
    }

    if (0 == strcmp (key, "add")) {
        if ((size > 0) && (size <= MAXNAMESIZE))
        {
            char *answerstring;
            answerstring = malloc (MAXANSWERSIZE);
            if (!answerstring)
                return MHD_NO;
            snprintf (answerstring, MAXANSWERSIZE, getPage(ADDUSER_PAGE), data);
            con_info->answerstring = answerstring;
        }
        else
            con_info->answerstring = NULL;

        return MHD_NO;
    }


    if (0 == strcmp (key, "remove")) {
        if ((size > 0) && (size <= MAXNAMESIZE))
        {
            char *answerstring;
            answerstring = malloc (MAXANSWERSIZE);
            if (!answerstring)
                return MHD_NO;
            snprintf (answerstring, MAXANSWERSIZE, getPage(DELETEUSER_PAGE), data);
            con_info->answerstring = answerstring;
        }
        else
            con_info->answerstring = NULL;

        return MHD_NO;
    }


    if (0 == strcmp (key, "tgid_delete")) {
        if ((size > 0) && (size <= MAXNAMESIZE))
        {
            char *answerstring;
            answerstring = malloc (MAXANSWERSIZE);
            if (!answerstring)
                return MHD_NO;
            delete_employee(&buf, &cnt, data);          // delete using id
            save_file(buf, cnt);
            read_database(&buf, &cnt);                          // read binary database file
            write_database_table(buf, cnt);                     // create temp table file
            OSCopyFile(TEMPLATE_DATABASE_PAGE, TEMP_PAGE);      // copy template to temp file, to be mofified
            updateDatabasePage();                               // update database to webpage
            snprintf (answerstring, MAXANSWERSIZE, getPage(DATABASE_PAGE), data);
            con_info->answerstring = answerstring;
        }
        else
            con_info->answerstring = NULL;

        return MHD_NO;
    }



    if (0 == strcmp (key, "name")) {
        if ((size > 0) && (size <= MAXNAMESIZE))
        {
            temp_storage = (employee*)malloc(sizeof(employee));
            strcpy(temp_storage->name, data);
        }
        else
            return MHD_NO;

        return MHD_YES;
    }


    if (0 == strcmp (key, "tgid")) {
        if ((size > 0) && (size <= MAXNAMESIZE))
        {
            strcpy(temp_storage->tgid, data);
        }
        else
            return MHD_NO;

        return MHD_YES;
    }

    if (0 == strcmp (key, "contact")) {
        if ((size > 0) && (size <= MAXNAMESIZE))
        {
            char *answerstring;
            answerstring = malloc (MAXANSWERSIZE);
            strcpy(temp_storage->contact, data);
            add_employee(&buf, &cnt);               // add to database
            save_file(buf, cnt);
            read_database(&buf, &cnt);                          // read binary database file
            write_database_table(buf, cnt);                     // create temp table file
            OSCopyFile(TEMPLATE_DATABASE_PAGE, TEMP_PAGE);      // copy template to temp file, to be mofified
            updateDatabasePage();                               // update database to webpage
            snprintf (answerstring, MAXANSWERSIZE, getPage(DATABASE_PAGE), data);
            con_info->answerstring = answerstring;
        }
        else
            con_info->answerstring = NULL;

        return MHD_NO;
    }

    return MHD_YES;
}

static void request_completed (void *cls, struct MHD_Connection *connection,
                               void **con_cls, enum MHD_RequestTerminationCode toe)
{
    struct connection_info_struct *con_info = *con_cls;
    (void)cls;         /* Unused. Silent compiler warning. */
    (void)connection;  /* Unused. Silent compiler warning. */
    (void)toe;         /* Unused. Silent compiler warning. */

    if (NULL == con_info)
        return;

    if (con_info->connectiontype == POST)
    {
        MHD_destroy_post_processor (con_info->postprocessor);
        if (con_info->answerstring)
            free (con_info->answerstring);
    }

    free (con_info);
    *con_cls = NULL;

    if (temp_storage)
    {
        free(temp_storage);
        temp_storage = NULL;
    }
}


static int answer_to_connection (void *cls, struct MHD_Connection *connection,
                                 const char *url, const char *method,
                                 const char *version, const char *upload_data,
                                 size_t *upload_data_size, void **con_cls)
{
    (void)cls;               /* Unused. Silent compiler warning. */
    (void)url;               /* Unused. Silent compiler warning. */
    (void)version;           /* Unused. Silent compiler warning. */

    if (NULL == *con_cls)
    {
        struct connection_info_struct *con_info;

        con_info = malloc (sizeof (struct connection_info_struct));
        if (NULL == con_info)
            return MHD_NO;
        con_info->answerstring = NULL;

        if (0 == strcmp (method, "POST"))
        {
            con_info->postprocessor =
                MHD_create_post_processor (connection, POSTBUFFERSIZE,
                                           iterate_post, (void *) con_info);

            if (NULL == con_info->postprocessor)
            {
                free (con_info);
                return MHD_NO;
            }

            con_info->connectiontype = POST;
        }
        else
            con_info->connectiontype = GET;

        *con_cls = (void *) con_info;

        return MHD_YES;
    }

    if (0 == strcmp (method, "GET"))
    {
        char *user;
        char *pass;
        int fail;
        int ret;
        struct MHD_Response *response;
        //basic authentication code
        pass = NULL;
        user = MHD_basic_auth_get_username_password (connection,
                &pass);
        fail = ( (NULL == user) ||
                 (0 != strcmp (user, "praveen")) ||
                 (0 != strcmp (pass, "pass") ) );
        if (NULL != user) MHD_free (user);
        if (NULL != pass) MHD_free (pass);
        if (fail)
        {
            const char *page = "<html><body>Go away.</body></html>";
            response =
                MHD_create_response_from_buffer (strlen (page), (void *) page,
                                                 MHD_RESPMEM_PERSISTENT);
            ret = MHD_queue_basic_auth_fail_response (connection,
                    "my realm",
                    response);
            return ret;
        }
        else
        {
            read_database(&buf, &cnt);                          // read binary database file
            write_database_table(buf, cnt);                     // create temp table file
            OSCopyFile(TEMPLATE_DATABASE_PAGE, TEMP_PAGE);      // copy template to temp file, to be mofified
            updateDatabasePage();                               // update database to webpage
            return send_page (connection, getPage(DATABASE_PAGE));
        }

    }

    if (0 == strcmp (method, "POST"))
    {
        struct connection_info_struct *con_info = *con_cls;

        if (*upload_data_size != 0)
        {
            MHD_post_process (con_info->postprocessor, upload_data,
                              *upload_data_size);
            *upload_data_size = 0;

            return MHD_YES;
        }
        else if (NULL != con_info->answerstring) {
            return send_page (connection, con_info->answerstring);
            return MHD_YES;
        }
    }

    return send_page (connection, errorpage);
}


int main ()
{
    struct MHD_Daemon *daemon;

    daemon = MHD_start_daemon (MHD_USE_AUTO | MHD_USE_INTERNAL_POLLING_THREAD, PORT, NULL, NULL,
                               &answer_to_connection, NULL,
                               MHD_OPTION_NOTIFY_COMPLETED, request_completed,
                               NULL, MHD_OPTION_END);
    if (NULL == daemon)
        return 1;

    printf("%s", "server started....");
    (void) getchar ();


    MHD_stop_daemon (daemon);

    return 0;
}

