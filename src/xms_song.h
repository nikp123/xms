#include <stdlib.h>
#include <stdbool.h>

// xms_handle
/**
 * Used for holding the state of the underlying media system
 * */
struct xms_handle;
/**
 * Obtain the XMS handle. It must be free-d after use.
 * */
struct xms_handle *xms_handle_get(void);
/**
 * Checks if the XMS handle can still be used.
 * */
bool               xms_handle_alive(struct xms_handle *handle);
/**
 * Frees the resources used by the XMS handle.
 * */
void               xms_handle_destroy(struct xms_handle *handle);



// xms_song
/**
 * XMS song object. It holds the information about the track currently playing.
 * In order to obtain the latest information xms_song_update is used.
 *
 * NOTE: Just like xms_handle, it must be free-d after use.
 * */
struct xms_song;

/**
 * Gets the currently playing song.
 * */
struct xms_song  *xms_song_get(struct xms_handle *handle);

/**
 * This function updates the information about the track. If a song change is detected,
 * it will automatically switch to that track.
 *
 * The return value indicates whether or not the song has been changed. (true == song changed)
 * */
bool              xms_song_update(struct xms_handle *handle, struct xms_song **song);

/**
 * Cleans up resources used by the song.
 * */
void              xms_song_destroy(struct xms_song *song);


/**
 * Get the number of milliseconds that have elapsed since the beginning of the track
 *
 * This value only gets updated by xms_song_update.
 *
 * Return value: The elapsed time in milliseconds.
 * */
long            xms_song_progress_ms_get(struct xms_song *song);

/**
 * Get the duration of the whole song in milliseconds. If this value is 0, that means
 * that the track has no specified lenght and is instead a audio stream, such as an
 * internet radio station.
 *
 * Return value: The lenght of the track in milliseconds.
 * */
long            xms_song_duration_ms_get(struct xms_song *song);

/**
 * Returns the title of the track as a C string. This string MAY NOT be modified.
 * */
const char     *xms_song_title_get(struct xms_song *song);

/**
 * Returns the name of the author as a C string. If there is no known author, a
 * NULL is returned. The returned string may not be modified.
 * */
const char     *xms_song_author_get(struct xms_song *song);

/**
 * Returns the name of the album of the track as a C string. If the album is not
 * known a NULL is returned. The returned string may not be modified.
 * */
const char     *xms_song_album_get(struct xms_song *song);

/**
 * Returns the genre of the song as a C string. If the genre is not known a NULL
 * is returned. The returned string may not be modified.
 * */
const char     *xms_song_genre_get(struct xms_song *song);

/**
 * This function fetches the artwork of the song.
 *
 * image_data specifies a pointer to a pointer which will point to the raw image data.
 * This image data may not be tampered with.
 *
 * image_mime is a pointer to a C-string that will be populated for you. This string
 * MAY NOT be tampered with. Example of mime-types: "image/jpeg"
 * Important to note: Some images may not be recognized by the underlying system, if
 * that's the case image_mime will be set to NULL.
 *
 * image_size is a pointer to a size_t which will contain the size of the image_data.
 *
 * In case no image has been found, image_data will be set to NULL, image_size
 * will be 0 and the return value will be false. If the opposite is true, then this
 * function will return a "true"
 * */
bool            xms_song_art_get(struct xms_song *song, const void **image_data,
                                 const char **image_mime, const size_t *image_size);

