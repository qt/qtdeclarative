#!/usr/bin/env python

#
# See CommandLineProcessor.print_usage() below.
#

# TODO: Consider checking "@non_strict_only".

import datetime
import fnmatch
import getopt
import os
import platform
import resource
import subprocess
import threading
import sys
import multiprocessing

### Settings & Command Line Processing ########################################

class Settings:
    interpreter = 'v4'
    interpreter_timeout = 60 # in s
    modes = ['jit', 'interpret']
    printers = ['txt', 'html']
    verbose = False
    input_dir = 'test262'
    # List of patterns with unix shell-style wildcards, see also
    #   http://docs.python.org/2/library/fnmatch.html
    # Example:
    #   blacklist_patterns = [
    #       "test262/console/harness/cth*"
    #   ]
    blacklist_patterns = []
    job_count = 1

class CommandLineProcessor:
    """ Process command line arguments and overwrite values in Settings."""
    def __init__(self, args):
        self.args = args
        self.valid_modes = ['aot', 'compile', 'jit', 'interpret', 'llvm-jit']
        self.valid_printers = ['txt', 'html']

    def print_usage(self):
        sys.stdout.write("""\
Usage: %s [-v] [-i interpreter] [-t timeout] [-d input_dir] [-p printers] [-m mode_list]

Run an interpreter with different modes (mode_list) on all *.js files in
input_dir and generate test reports based on the exit code and the output of
the interpreter.

Test reports are generated in TXT and HTML format. Paths to generated reports
are printed upon a finished run.

Options:
  -i, --interpreter  Use the specified interpreter (default: '%s').
  -t, --timeout      Timeout in seconds for the interpreter to process a single
                     file (default: '%s').
  -d, --input-dir    *.js files will be searched in input_dir. (default: '%s').
  -m, --modes        Run the interpreter with specified modes. mode_list is a
                     comma separated list (valid: '%s',
                     default: '%s').
  -p, --printers     Print results by specified printers. printers is a comma
                     separated list (valid: '%s', default: '%s').
  -j, --jobs         Allow N jobs at once (default: %s).
  -v, --verbose      Print some more information, e.g. files are being
                     processed (default: not enabled).

""" % (
            sys.argv[0],
            Settings.interpreter,
            Settings.interpreter_timeout,
            Settings.input_dir,
            ",".join(self.valid_modes),
            ",".join(Settings.modes),
            ",".join(self.valid_printers),
            ",".join(Settings.printers),
            Settings.job_count
        ))

    def run(self):
        try:
            options, arguments = getopt.getopt(self.args, "hvm:i:d:p:t:j:",
                ["help", "verbose", "modes=", "interpreter=", "input-dir=", "printers=", "timeout=", "jobs="])
        except getopt.error, msg:
            sys.stdout.write("For help use -h, --help.")
            sys.exit(2)
        for option, argument in options:
            if option in ("-h", "--help"):
                self.print_usage()
                sys.exit(0)
            elif option in ("-m", "--modes"):
                modes = filter(None, argument.split(','))
                if not modes:
                        sys.stdout.write("No modes provided.")
                        sys.stdout.write("For help use -h, --help.")
                        sys.exit(2)
                for mode in modes:
                    if not mode in self.valid_modes:
                        sys.stdout.write("Mode '%s' is invalid." %(mode))
                        sys.stdout.write("For help use -h, --help.")
                        sys.exit(2)
                Settings.modes = modes
            elif option in ("-p", "--printers"):
                printers = filter(None, argument.split(','))
                if not printers:
                        sys.stdout.write("No printers provided.")
                        sys.stdout.write("For help use -h, --help.")
                        sys.exit(2)
                for printer in printers:
                    if not printer in self.valid_printers:
                        sys.stdout.write("Printer '%s' is invalid." %(printer))
                        sys.stdout.write("For help use -h, --help.")
                        sys.exit(2)
                Settings.printers = printers
            elif option in ("-i", "--interpreter"):
                Settings.interpreter = argument
            elif option in ("-d", "--input-dir"):
                Settings.input_dir = argument
            elif option in ("-t", "--interpreter-timeout"):
                Settings.interpreter_timeout = int(argument)
            elif option in ("-v", "--verbose"):
                Settings.verbose = True
            elif option in ("-j", "--jobs"):
                Settings.job_count = int(argument)
            else:
                sys.stdout.write("Unknown option '%s'." %(option))
                sys.stdout.write("For help use -h, --help.")
                sys.exit(2)

### Utils #####################################################################

def check_candidates(candidates):
    """ Return files passing the black list Settings.blacklist_patterns. """
    input_files = []
    for candidate in candidates:
        is_blacklisted = False
        for p in Settings.blacklist_patterns:
            if fnmatch.fnmatch(candidate, p):
                if Settings.verbose:
                    sys.stdout.write("Will ignore '%s' since it matches black list filter '%s'.\n" %(candidate, p))
                is_blacklisted = True
                break
        if not is_blacklisted:
            input_files.append(candidate)
    return input_files

def generic_footer(time_started, time_finished, total_pass, total_passn, total_fail, total_gout, total_time):
    totals_processed = total_pass + total_passn + total_fail + total_gout
    return """
Key:
 PASS:  Exit code is zero and _no_ output generated.
 PASSN: Exit code is non zero and *.js contains "@negative".
 GOUT:  Exit code is zero and output generated.
 FAIL:  Exit code is non zero.
 TIME:  Interpreter does not return within timeout of %d seconds.

Totals: %d processed - %d passed, %d passn'ed, %d failed, %d gout'ed, %d timeout'ed.

Started: %s
Finished: %s""" %(
        Settings.interpreter_timeout,
        totals_processed, total_pass, total_passn, total_fail, total_gout, total_time,
        time_started.strftime("%A, %d. %B %Y %I:%M%p"),
        time_finished.strftime("%A, %d. %B %Y %I:%M%p")
    )

class Command(object):
    def __init__(self, cmd):
        self.cmd = cmd
        self.process = None

        self.finished_within_timeout = False
        self.exit_code = None
        self.stdout = None
        self.stderr = None

    def run(self, timeout):
        def target():
            my_env = os.environ.copy()
            my_env['IN_TEST_HARNESS'] = '1'
            self.process = subprocess.Popen(self.cmd, stdout=subprocess.PIPE,
                stderr=subprocess.PIPE, preexec_fn=self.__set_limits, env=my_env)
            (self.stdout, self.stderr) = self.process.communicate()

        thread = threading.Thread(target=target)
        thread.start()
        thread.join(timeout)
        if thread.is_alive():
            self.process.terminate()
            thread.join()
        else:
            self.finished_within_timeout = True
            self.exit_code = self.process.returncode

    def __set_limits(self):
        if platform.system() in ('Linux', 'Darwin'):
            one_gigabyte = 1024*1024*1024
            resource.setrlimit(resource.RLIMIT_AS, (one_gigabyte, one_gigabyte))
            resource.setrlimit(resource.RLIMIT_CORE, (0, 0))

class InterpreterRunnerInfo:
    def __init__(self, info):
        self.info = info

class InterpreterResult:
    def __init__(self, file_path, finished_within_timeout, code, output_stdout, output_stderr):
        self.file_path = file_path
        self.finished_within_timeout = finished_within_timeout
        self.code = code
        self.output_stdout= output_stdout
        self.output_stderr = output_stderr

def executeCommand(data):
    interpreterRunner, input_file = data
    if not os.path.exists(input_file):
        sys.stderr.write("Warning:  File '%s' does not exist anymore for interpreter '%s'.\n"
            %(input_file, interpreterRunner.runner_info.info))
        return
    if Settings.verbose:
        sys.stdout.write("  %s\n" %(input_file))
    command = ['./' + Settings.interpreter] + interpreterRunner.extra_args + [input_file]

    process = Command(command)
    process.run(timeout=Settings.interpreter_timeout)
    if not process.finished_within_timeout:
        sys.stderr.write("Warning: Interpreter '%s' did not finish within %d seconds processing file '%s'.\n"
            %(interpreterRunner.runner_info.info, Settings.interpreter_timeout, input_file))
    result = InterpreterResult(
        input_file,
        process.finished_within_timeout,
        process.exit_code,
        process.stdout,
        process.stderr
    )
    return result

class InterpreterRunner:
    def __init__(self, input_files, label, extra_args, printers):
        self.input_files = input_files
        self.label = label
        self.extra_args = extra_args
        self.printers = printers
        self.runner_info = InterpreterRunnerInfo(Settings.interpreter + ' ' + " ".join(self.extra_args))

    def __printers_begin(self):
        for printer in self.printers:
            printer.reset()
            printer.set_output_file("testresults_%s.%s" %(os.path.basename(Settings.interpreter), self.label))
            printer.set_runner_info(self.runner_info)
            printer.print_header(datetime.datetime.now())

    def __printers_end(self):
        for printer in self.printers:
            printer.print_footer(datetime.datetime.now())

    def __printers_do(self, result):
        for printer in self.printers:
            printer.print_print(result)

    def run(self):
        sys.stdout.write("Running interpreter '%s %s' for each test file.\n"
                %(Settings.interpreter, " ".join(self.extra_args)))

        self.__printers_begin()
        processPool = multiprocessing.Pool(processes = Settings.job_count)
        results = processPool.map(executeCommand, [(self, input_file) for input_file in self.input_files])
        for result in results:
            self.__printers_do(result) # TODO we may put it to the executeCommand, if it is safe
        self.__printers_end()

### Printers ##################################################################

class AbstractInterpreterPrinter:
    def __init__(self, file_suffix):
        self.file_suffix = file_suffix
        self.reset()

    def reset(self):
        self.count_pass = 0
        self.count_passn = 0
        self.count_fail = 0
        self.count_gout = 0
        self.count_time = 0

    def set_output_file(self, file_path):
        self.output_file = open(file_path + self.file_suffix, 'w')

    def set_runner_info(self, runner_info):
        self.runner_info = runner_info

    def print_header(self, time_started):
        """Must be called by derived method to ensure correct totals."""
        self.reset()
        self.time_started = time_started

    def print_print(self, result):
        """ Returns result marker and update totals.
            Must be called by derived method to ensure correct totals. """
        if result.finished_within_timeout:
            if result.code != 0:
                if '@negative' in open(result.file_path, 'r').read(1024):
                    result_marker = "PASSN"
                    self.count_passn = self.count_passn + 1
                else:
                    result_marker = "FAIL"
                    self.count_fail = self.count_fail + 1
            else:
                if result.output_stdout or result.output_stderr:
                    result_marker = "GOUT"
                    self.count_gout = self.count_gout + 1
                else:
                    result_marker = "PASS"
                    self.count_pass = self.count_pass + 1
        else:
            result_marker = "TIME"
            self.count_time = self.count_time + 1
        return result_marker

    def print_footer(self, time_finished):
        raise NotImplementedError, "Implement me"

    def generic_footer(self):
        return generic_footer(self.time_started, self.time_finished, self.count_pass,
            self.count_passn, self.count_fail, self.count_gout, self.count_time)

class InterpreterTextPrinter(AbstractInterpreterPrinter):
    def __init__(self):
        AbstractInterpreterPrinter.__init__(self, '.txt')

    def print_header(self, time_started):
        AbstractInterpreterPrinter.print_header(self, time_started)
        self.output_file.write("********* Start testing of '%s' *********\n" %(Settings.interpreter))
        self.output_file.write("Config: %s\n" %(self.runner_info.info))
        self.output_file.flush()

    def print_print(self, result):
        result_marker = AbstractInterpreterPrinter.print_print(self, result)
        self.output_file.write("%-5s %-26s %s\n" %(result_marker, os.path.basename(result.file_path),
            result.file_path))
        self.output_file.flush()

    def print_footer(self, time_finished):
        self.time_finished = time_finished
        self.output_file.write("********* Finished testing of '%s' *********\n" %(Settings.interpreter))
        self.output_file.write(AbstractInterpreterPrinter.generic_footer(self))
        self.output_file.flush()
        sys.stdout.write("Generated '%s'.\n" %(self.output_file.name))

class InterpreterHtmlPrinter(AbstractInterpreterPrinter):
    def __init__(self):
        AbstractInterpreterPrinter.__init__(self, '.html')

    def print_header(self, time_started):
        AbstractInterpreterPrinter.print_header(self, time_started)

        preamble = ("""\
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN"
       "http://www.w3.org/TR/html4/loose.dtd">
<html>
<head>
<title>Test results for '%s'</title>
<style type="text/css">
    key { font-size: smaller; }
    table { border-collapse: collapse; }
    body { font-family: sans-serif; }
    th { padding: 4px; text-align: left; }
    td { padding-left: 3px; padding-right: 3px; }
    .pass { text-align: center; background-color:#00FF00; }
    .passn { text-align: center; background-color:#00DE00; }
    .fail { text-align: center; background-color:#FF4649; }
    .gout { text-align: center; background-color:#FFDCA8; }
    .smaller { font-size: font:1.2em/1.5em; }
</style>
</head>
<body>
<h1>Test results for '%s'</h1>

<table border="1" width="100%%">
 <tr>
  <th style="text-align: center">Result</th>
  <th>Test File</th>
  <th>Interpreter Output (stdout <span style="color: red">stderr</span>)</th>
  <th>Full Test File Path</th>
 </tr>

""" %(
    self.runner_info.info,
    self.runner_info.info
    ))
        self.output_file.write(preamble)
        self.output_file.flush()

    def print_print(self, result):
        result_marker = AbstractInterpreterPrinter.print_print(self, result)
        has_output = result.output_stdout or result.output_stderr
        output = result.output_stdout + "<span style='color: red'> " + result.output_stderr + "</span>"
        if not has_output:
            output = ""
        row = ("""\
     <tr>
      <td class="%s">%s</td>
      <td>%s</td>
      <td>%s</td>
      <td>%s</td>
     </tr>""" %(
        result_marker.lower(),
        result_marker,
        os.path.basename(result.file_path),
        output,
        result.file_path
        ))

        self.output_file.write(row)
        self.output_file.flush()

    def print_footer(self, time_finished):
        self.time_finished = time_finished
        footer = ("""\
</table>
<pre>
%s
</pre>
</body>
</html>
""" % (AbstractInterpreterPrinter.generic_footer(self)))
        self.output_file.write(footer)
        sys.stdout.write("Generated '%s'.\n" %(self.output_file.name))

### Main ######################################################################

def main():
    # Handle command line options
    cmdline_processor = CommandLineProcessor(sys.argv[1:])
    cmdline_processor.run()

    # Sanity checks
    if not os.path.exists(Settings.interpreter):
        sys.stderr.write("Error: Interpreter '%s' not found.\n" %(Settings.interpreter))
        sys.exit(3)
    if not os.path.exists(Settings.input_dir):
        sys.stderr.write("Error: Directory with test data '%s' not found.\n" %(Settings.input_dir))
        sys.exit(3)

    # Collect files
    candidates = []
    for root, dirnames, filenames in os.walk(Settings.input_dir):
      for filename in fnmatch.filter(filenames, '*.js'):
          candidates.append(os.path.join(root, filename))
    candidates.sort()
    input_files = check_candidates(candidates)

    if not input_files:
        sys.stderr.write("Error: No relevant test files found in '%s'.\n" %(Settings.input_dir))
        sys.exit(3)
    if Settings.verbose:
        sys.stdout.write("Will process %s files for each interpreter mode.\n" %(len(input_files)))

    # Run interpreter
    printers = []
    for printer in Settings.printers:
        if printer == "txt":
            printers.append( InterpreterTextPrinter() )
        elif printer == "html":
            printers.append( InterpreterHtmlPrinter() )

    for mode in Settings.modes:
        runner = InterpreterRunner(input_files, mode, ["--" + mode], printers)
        runner.run()

if __name__ == "__main__":
    main()
