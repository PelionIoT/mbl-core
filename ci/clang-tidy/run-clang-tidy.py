#!/usr/bin/env python3

import argparse
import json
import pathlib
import shutil
import subprocess
import sys

SCRIPT_DIR = pathlib.Path(__file__).absolute().parent


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--projectdata",
        nargs="+",
        type=dict,
        help="Location of json file containing extra header info",
        default=_load_conf()
    )
    return parser.parse_args()


def _load_conf():
    conf_file_path = SCRIPT_DIR / "clang-tidy-info.json"
    return json.loads(conf_file_path.read_text())


class ClangTidyRunner:
    def __init__(self, project_path, project_data):
        self.data = project_data
        self.project_path = pathlib.Path(SCRIPT_DIR, project_path).absolute()
        self.extra_flags = ""
        if "headers" in self.data:
            self._fetch_extra_headers()

    def _fetch_extra_headers(self):
        for header_info in self.data["headers"]:
            repo_url, header_loc = header_info["repo"].split(";")
            repo_path = SCRIPT_DIR / "dst"
            subprocess.run(["git", "clone", repo_url, str(repo_path)], check=True)
            try:
                hdr_dst_dir = self.project_path / header_info["destdir"]
                hdr_path = pathlib.Path(SCRIPT_DIR, "dst", header_loc).absolute()
                shutil.copytree(str(hdr_path), hdr_dst_dir)
                self.extra_flags = "-DCMAKE_{lang}_FLAGS=-I {incdir}".format(
                    incdir=str(hdr_dst_dir),
                    lang=self.data["lang"]
                )
            finally:
                shutil.rmtree(str(repo_path))

    def run(self):
        subprocess.run(
            [
                "cmake",
                '"-DCMAKE_{}_CLANG_TIDY=clang-tidy;-checks=*"'.format(
                    self.data["lang"]
                ),
                "-DCMAKE_INSTALL_LIBDIR={}".format(SCRIPT_DIR),
                "-DCMAKE_INSTALL_BINDIR={}".format(SCRIPT_DIR),
                self.extra_flags,
                "-S{}".format(str(self.project_path)),
                "-B{}".format(str(self.project_path)),
                "--trace-expand",
            ],
            check=True
        )
        subprocess.run(["make", "-C", str(self.project_path)], check=True)


def main(args):
    for project_path, project_data in args.projectdata.items():
        ClangTidyRunner(project_path, project_data).run()


if __name__ == "__main__":
    sys.exit(main(parse_args()))
