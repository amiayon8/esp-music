import argparse
from pathlib import Path
import re
import shutil
import sys
from typing import Dict, Optional
import yt_dlp

OUTPUT_DIRECTORY = "./Music"
AUDIO_FORMAT = "flac"

PLAYLISTS: Dict[str, str] = {

}

ILLEGAL_CHARACTERS_PATTERN = re.compile(r'[<>:"/\\|?*\x00-\x1f]')

def sanitize_folder_name(name: str) -> str:
    cleaned = ILLEGAL_CHARACTERS_PATTERN.sub("_", name)
    cleaned = cleaned.strip(" .")
    if not cleaned:
        return "Untitled_Playlist"
    return cleaned

def ensure_folder_cover_art(playlist_directory: Path) -> None:
    cover_file = playlist_directory / "cover.jpg"
    if cover_file.is_file():
        return
    image_candidates = list(playlist_directory.glob("*.jpg")) + list(playlist_directory.glob("*.png"))
    for candidate in image_candidates:
        if candidate.name != "cover.jpg":
            shutil.copyfile(candidate, cover_file)
            return

def build_ytdl_configuration(
    playlist_directory: Path,
    audio_format: str,
    cookies_path: Optional[str],
    browser_for_cookies: Optional[str],
) -> dict:
    archive_file_path = playlist_directory / ".download_archive.txt"
    output_template = str(playlist_directory / "%(title)s.%(ext)s")

    postprocessors = [
        {
            "key": "FFmpegThumbnailsConvertor",
            "format": "jpg",
        },
        {
            "key": "FFmpegMetadata",
            "add_metadata": True,
        },
        {
            "key": "EmbedThumbnail",
        },
    ]

    if audio_format.lower() != "best":
        postprocessors.insert(
            0,
            {
                "key": "FFmpegExtractAudio",
                "preferredcodec": audio_format.lower(),
                "preferredquality": "0",
            },
        )

    configuration = {
        "format": "bestaudio/best",
        "outtmpl": output_template,
        "download_archive": str(archive_file_path),
        "writethumbnail": True,
        "ignoreerrors": True,
        "no_warnings": False,
        "quiet": False,
        "postprocessors": postprocessors,
        "windowsfilenames": True,
    }

    if cookies_path:
        configuration["cookiefile"] = cookies_path
    elif browser_for_cookies:
        configuration["cookiesfrombrowser"] = (browser_for_cookies,)

    return configuration

def sync_single_playlist(
    playlist_name: str,
    playlist_url: str,
    base_output_directory: Path,
    audio_format: str,
    cookies_path: Optional[str],
    browser_for_cookies: Optional[str],
) -> bool:
    if not playlist_url or not playlist_url.strip():
        print(f"Skipping '{playlist_name}': URL is empty.")
        return False

    folder_name = sanitize_folder_name(playlist_name)
    playlist_directory = base_output_directory / folder_name
    playlist_directory.mkdir(parents=True, exist_ok=True)

    print(f"\n==================================================")
    print(f"Syncing Playlist: {playlist_name}")
    print(f"Destination: {playlist_directory}")
    print(f"URL: {playlist_url}")
    print(f"Quality: Lossless best available ({audio_format})")
    print(f"==================================================")

    configuration = build_ytdl_configuration(
        playlist_directory=playlist_directory,
        audio_format=audio_format,
        cookies_path=cookies_path,
        browser_for_cookies=browser_for_cookies,
    )

    try:
        with yt_dlp.YoutubeDL(configuration) as ydl_instance:
            return_code = ydl_instance.download([playlist_url])
        ensure_folder_cover_art(playlist_directory)
        return return_code == 0
    except Exception as error:
        print(f"Error syncing playlist '{playlist_name}': {error}")
        return False

def sync_all_playlists(
    playlist_dictionary: Dict[str, str],
    base_output_directory: Path,
    audio_format: str,
    cookies_path: Optional[str],
    browser_for_cookies: Optional[str],
) -> None:
    base_output_directory.mkdir(parents=True, exist_ok=True)
    total_count = len(playlist_dictionary)
    successful_count = 0

    print(f"Starting synchronization of {total_count} playlist(s)...")
    print(f"Target directory: {base_output_directory.resolve()}")

    for name, url in playlist_dictionary.items():
        success = sync_single_playlist(
            playlist_name=name,
            playlist_url=url,
            base_output_directory=base_output_directory,
            audio_format=audio_format,
            cookies_path=cookies_path,
            browser_for_cookies=browser_for_cookies,
        )
        if success:
            successful_count += 1

    print(f"\nSynchronization completed: {successful_count}/{total_count} playlist(s) processed successfully.")

def parse_command_line_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Download and sync YouTube Music playlists into structured folders with lossless / best available quality audio."
    )
    parser.add_argument(
        "-o",
        "--output",
        default=OUTPUT_DIRECTORY,
        help=f"Base folder to save synced playlists into. Default: {OUTPUT_DIRECTORY}",
    )
    parser.add_argument(
        "-f",
        "--format",
        default=AUDIO_FORMAT,
        choices=["flac", "best", "wav", "m4a", "opus", "mp3"],
        help=f"Audio format to save. Default: {AUDIO_FORMAT}",
    )
    parser.add_argument(
        "--cookies",
        default=None,
        help="Path to cookies.txt file for authenticated access.",
    )
    parser.add_argument(
        "--cookies-from-browser",
        default=None,
        help="Name of browser to extract cookies from (e.g. chrome, firefox, edge).",
    )
    return parser.parse_args()

def main() -> None:
    arguments = parse_command_line_arguments()
    base_output_directory = Path(arguments.output)

    if not PLAYLISTS:
        print("The PLAYLISTS dictionary is empty.")
        print("Please open sync_playlists.py and add your playlist names and links to PLAYLISTS:")
        print('PLAYLISTS: Dict[str, str] = {')
        print('    "My Playlist Name": "https://music.youtube.com/playlist?list=...",')
        print('}')
        sys.exit(0)

    sync_all_playlists(
        playlist_dictionary=PLAYLISTS,
        base_output_directory=base_output_directory,
        audio_format=arguments.format,
        cookies_path=arguments.cookies,
        browser_for_cookies=arguments.cookies_from_browser,
    )

if __name__ == "__main__":
    main()
