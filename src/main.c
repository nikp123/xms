#include <stdio.h>
#include <unistd.h>
#include "xms_song.h"

int main(int argc, char *argv[]) {
	char *song_title, *song_author;

	struct xms_handle *handle;
	struct xms_song   *song;

	handle = xms_handle_get();
	if(handle == NULL)
		return -1;

	song = xms_song_get(handle);
	if(song == NULL)
		return -1;

	while(1) {
		xms_song_update(handle, &song);

		if(song == NULL) {
			printf("No song currently playing!\n");
		} else {
			printf("%s - %s (%lu:%lu)\n", xms_song_author_get(song), xms_song_title_get(song),
				xms_song_progress_ms_get(song), xms_song_duration_ms_get(song));
		}
		usleep(1000000);
	}

	xms_song_destroy(song);
	xms_handle_destroy(handle);
}
