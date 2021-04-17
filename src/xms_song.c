#include <mpd/status.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include <math.h>

#include <mpd/song.h>
#include <mpd/player.h>
#include <mpd/tag.h>
#include <mpd/client.h>

#include <unistd.h>

#include "xms_song.h"

char *safe_strdup(const char *input) {
	if(input != NULL) {
		return strdup(input);
	} else return NULL;
}

// X11 Music Stats = xms

// splits assume that the song is formatted in the usual monstercat-style 
// title format
const char *xms_song_title_split = " - ";

bool xms_song_title_split_exists(const char *song_title) {
	const char *split = xms_song_title_split;

	for(int i = 0; i < strlen(song_title)-strlen(split); i++) {
		if(!strncmp(&song_title[i], split, strlen(split))) {
			return true;
		}
	}

	return false;
}

/**
 * Assumes that the song title is staticly allocated, which would SIGSEGV
 * this program. So let's just allocate a new pointer title instead.
 **/
char *xms_song_title_split_title_get(const char *song_title) {
	const char *split = xms_song_title_split;

	char *song_title_new;
	for(int i = 0; i < strlen(song_title)-strlen(split); i++) {
		if(!strncmp(&song_title[i], split, strlen(split))) {
			i+=strlen(split); // skip the split itself
			song_title_new = malloc(strlen(&song_title[i])+1);
			strcpy(song_title_new, &song_title[i]);
			return song_title_new;
		}
	}

	return NULL;
}

char *xms_song_title_split_author_get(const char *song_title) {
	const char *split = xms_song_title_split;

	char *song_author;
	for(int i = 0; i < strlen(song_title)-strlen(split); i++) {
		if(!strncmp(&song_title[i], split, strlen(split))) {
			song_author = malloc(sizeof(char)*(i+1));
			memcpy(song_author, song_title, i);
			song_author[i] = '\0';

			return song_author;
		}
	}

	return NULL;
}

struct xms_handle {
	struct mpd_connection *connection;
};

struct xms_handle *xms_handle_get() {
	struct xms_handle *xms_handle = malloc(sizeof(struct xms_handle));

	// TODO: Make this changable
	xms_handle->connection = mpd_connection_new("password@localhost", 6600, 1000);
	if(mpd_connection_get_error(xms_handle->connection) != MPD_ERROR_SUCCESS) {
		fprintf(stderr, "Failed to connect!\n");
		return NULL;
	}

	return xms_handle;
}

bool xms_handle_alive(struct xms_handle *handle) {
	if(mpd_connection_get_error(handle->connection) != MPD_ERROR_SUCCESS)
		return false;

	return true;
}

void xms_handle_destroy(struct xms_handle *handle) {
	mpd_connection_free(handle->connection);
	free(handle);
}

struct xms_song {
	struct mpd_song *mpd_song_handle;
	struct mpd_status *status;
	int    mpd_song_id;
	char   *mpd_title;

	bool streaming;

	char *author, *title, *album, *genre;
	char *uri;

	bool playing;

	long progress, duration;

	size_t image_size;
	void *image_data;
	char *image_mime;
};

enum xms_song_image_type {
	XMS_SONG_INFO_IMAGE_DEFAULT,
	XMS_SONG_INFO_IMAGE_ALBUMART,
};

bool xms_song_image_get(struct xms_handle *handle, 
							struct xms_song *song,
							enum xms_song_image_type image_type) {
	struct mpd_pair *pair;
	bool image_success;
	char *image_command;

	// reset all variables to a safe default
	size_t image_parsed = 0;
	song->image_size = 0;
	song->image_mime = NULL;
	song->image_data = NULL;

	switch(image_type) {
		case XMS_SONG_INFO_IMAGE_DEFAULT:
			image_command = "readpicture"; 
			break;
		case XMS_SONG_INFO_IMAGE_ALBUMART:
			image_command = "albumart";
			break;
		default:
			fprintf(stderr, "Invalid image type '%d' in %s:%d\n",
					image_type, __FILE__, __LINE__);
			return false;
	}

	do {
		char *offset_string = malloc(ceil(log10((double)song->image_size+1)+2));
		sprintf(offset_string, "%lu", image_parsed); 

		mpd_send_command(handle->connection, image_command, song->uri,
				offset_string, NULL);
		free(offset_string);

		while((pair = mpd_recv_pair(handle->connection)) != NULL) {
			if(!strcmp(pair->name, "size")) {
				if(song->image_size == 0) {
					song->image_size = atoi(pair->value);
					song->image_data = malloc(song->image_size);
				}
			} else if(!strcmp(pair->name, "type")) {
				if(song->image_mime == NULL)
					song->image_mime = strdup(pair->value);
			} else if(!strcmp(pair->name, "binary")) {
				size_t chunk_size = atoi(pair->value);
				mpd_return_pair(handle->connection, pair);

				image_success = mpd_recv_binary(handle->connection,
						song->image_data+image_parsed, chunk_size);

				assert(image_success);

				image_parsed += chunk_size;
				continue; // apparently mpd_recv_pair fails if you dont flush the command buffer
			}
			mpd_return_pair(handle->connection, pair);
		}
	} while(image_parsed < song->image_size);

	if(song->image_size == 0)
		return false;

	return true;
}

/**
 * Give this function pointers to strings which will be filled accordingly
 * */
struct xms_song *xms_song_get(struct xms_handle *handle) {
	struct xms_song *song = malloc(sizeof(struct xms_song));

	song->mpd_song_handle = mpd_run_current_song(handle->connection);
	if(song->mpd_song_handle == NULL) {
		fprintf(stderr, "No song currently playing!\n");
		return NULL;
	}

	bool song_title_has_author = false;

	const char *title, *author;

	if(mpd_song_get_tag(song->mpd_song_handle, MPD_TAG_ARTIST, 0))
		song_title_has_author = true;

	if(song_title_has_author) {
		title  = mpd_song_get_tag(song->mpd_song_handle, MPD_TAG_TITLE, 0);
		author = mpd_song_get_tag(song->mpd_song_handle, MPD_TAG_ARTIST, 0);

		song->author = strdup(author);
		song->title  = strdup(title);
	} else {
		title = mpd_song_get_tag(song->mpd_song_handle, MPD_TAG_TITLE, 0);

		if(xms_song_title_split_exists(title)) {
			song->author = xms_song_title_split_author_get(title);
			song->title  = xms_song_title_split_title_get(title);
		} else {
			// Warning: Author unknown
			song->author = strdup("");
			song->title = strdup(title);
		}
	}

	song->mpd_title = strdup(title);

	song->uri = strdup(mpd_song_get_uri(song->mpd_song_handle));

	song->genre = safe_strdup(mpd_song_get_tag(song->mpd_song_handle, MPD_TAG_GENRE, 0));
	song->album = safe_strdup(mpd_song_get_tag(song->mpd_song_handle, MPD_TAG_ALBUM, 0));

	song->duration = mpd_song_get_duration_ms(song->mpd_song_handle);

	if(!xms_song_image_get(handle, song, XMS_SONG_INFO_IMAGE_DEFAULT))
		xms_song_image_get(handle, song, XMS_SONG_INFO_IMAGE_ALBUMART);

	mpd_send_status(handle->connection);
	song->status = mpd_recv_status(handle->connection);

	// a dirty hack just to keep MPD from crashing
	if(song->status == NULL) {
		mpd_connection_clear_error(handle->connection);
		song->status = mpd_status_begin();
		song->streaming = true;
	}

	song->mpd_song_id = mpd_status_get_song_id(song->status);

	return song;
}

// true indicates that the song has been update
bool xms_song_update(struct xms_handle *handle, struct xms_song **song) {
	bool update_song = false;

	// a dirty hack just to keep MPD from crashing
	if(!(*song)->streaming) {
		mpd_status_free((*song)->status);

		mpd_send_status(handle->connection);
		(*song)->status = mpd_recv_status(handle->connection);

		int new_song_id = mpd_status_get_song_id((*song)->status);

		// Update by MPD's internal ID
		if(new_song_id != (*song)->mpd_song_id)
			update_song = true;
	} else {
		struct mpd_pair *pair;

		mpd_send_command(handle->connection, "currentsong", NULL);
		while((pair = mpd_recv_pair(handle->connection)) != NULL) {
			if(!strcmp(pair->name, "Title")) {
				// Update by last modified date
				if(strcmp(pair->value, (*song)->mpd_title))
					update_song = true;
			}
			mpd_return_pair(handle->connection, pair);
		}
	}

	if(update_song) {
		xms_song_destroy((*song));
		*song = xms_song_get(handle);
		return true;
	}

	(*song)->progress = mpd_status_get_elapsed_ms((*song)->status);
	return false;
}

long xms_song_progress_ms_get(struct xms_song *song) {
	return song->progress;
}

long xms_song_duration_ms_get(struct xms_song *song) {
	return song->duration;
}

bool xms_song_art_get(struct xms_song *song, const void **image_data,
		const char **image_mime, const size_t *image_size) {
	if(song->image_size == 0)
		return false;

	image_data = (const void**)&song->image_data;
	image_mime = (const char**)&song->image_mime;
	image_size = (const size_t*)&song->image_size;
	return true;
}

const char *xms_song_album_get(struct xms_song *song) {
	return song->album;
}

const char *xms_song_genre_get(struct xms_song *song) {
	return song->genre;
}

const char *xms_song_title_get(struct xms_song *song) {
	return song->title; 
}

const char *xms_song_author_get(struct xms_song *song) {
	return song->author;
}

void xms_song_destroy(struct xms_song *song) {
	mpd_song_free(song->mpd_song_handle);
	mpd_status_free(song->status);
	free(song->mpd_title);

	if(song->image_size) {
		if(song->image_mime != NULL) {
			free(song->image_mime);
		}
		free(song->image_data);
	}

	free(song->genre);
	free(song->album);
	free(song->author);
	free(song->title);
	free(song->uri);
	free(song);
}
