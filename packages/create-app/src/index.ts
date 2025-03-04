import { Command } from 'commander';
import fs from 'fs-extra';
import path from 'path';
import updateCheck from 'update-check';
import prompts from 'prompts';
import { bold, cyan, green, red, yellow } from 'picocolors';
import { validateNpmName, isFolderEmpty, getPkgManager, install } from './helpers';
import { bindingVersion, cccCoreVersion, coreVersion, testtoolVersion } from './config';

const packageJson = require('../package.json');

let projectName = '';

const handleSigTerm = (): void => process.exit(0);

process.on('SIGINT', handleSigTerm);
process.on('SIGTERM', handleSigTerm);

const onPromptState = (state: any): void => {
  if (state.aborted) {
    // If we don't re-enable the terminal cursor before exiting
    // the program, the cursor will remain hidden
    process.stdout.write('\x1B[?25h');
    process.stdout.write('\n');
    process.exit(1);
  }
};

const program = new Command(packageJson.name)
  .version(
    packageJson.version,
    '-v, --version',
    `Output the current version of ${packageJson.name}.`
  )
  .argument('[directory]')
  .usage('[directory] [options]')
  .helpOption(
    '-h, --help',
    'Display this help message.'
  )
  .option(
    '--skip-install',
    'Explicitly tell the CLI to skip installing packages.'
  )
  .action((name: string) => {
    // Commander does not implicitly support negated options. When they are used
    // by the user they will be interpreted as the positional argument (name) in
    // the action handler. See https://github.com/tj/commander.js/pull/1355
    if (name && !name.startsWith('--no-')) {
      projectName = name;
    }
  })
  .allowUnknownOption()
  .parse(process.argv);

const opts = program.opts();
const { args } = program;

const packageManager = getPkgManager();

function updatePackageJson1(projectPath: string) {
  const packageJsonPath = path.join(projectPath, 'package/on-chain-script/package.json');
  let json: any = '';
  if (fs.pathExistsSync(packageJsonPath)) {
    try {
      json = fs.readJsonSync(packageJsonPath);
      json.name = projectName;
      json.devDependencies["ckb-testtool"] = testtoolVersion
      json.dependencies["@ckb-js-std/bindings"] = bindingVersion
      json.dependencies["@ckb-js-std/core"] = coreVersion
      fs.writeJsonSync(packageJsonPath, json, { spaces: 2 });
      console.log(green(`Updated ${projectName}/package.json.`));
    } catch (error: any) {
      console.error(red(`Failed to update package.json: ${error.message}`));
      process.exit(1);
    }
  } else {
    console.error(
      red(`Could not find package.json in the template. Make sure your template includes a package.json.`)
    );
    process.exit(1);
  }
}

function updatePackageJson2(projectPath: string) {
  const packageJsonPath = path.join(projectPath, 'package/on-chain-script-test/package.json');
  let json: any = '';
  if (fs.pathExistsSync(packageJsonPath)) {
    try {
      json = fs.readJsonSync(packageJsonPath);
      json.name = projectName;
      json.devDependencies["ckb-testtool"] = testtoolVersion
      json.devDependencies["@ckb-ccc/core"] = cccCoreVersion

      fs.writeJsonSync(packageJsonPath, json, { spaces: 2 });
      console.log(green(`Updated ${projectName}/package.json.`));
    } catch (error: any) {
      console.error(red(`Failed to update package.json: ${error.message}`));
      process.exit(1);
    }
  } else {
    console.error(
      red(`Could not find package.json in the template. Make sure your template includes a package.json.`)
    );
    process.exit(1);
  }
}

async function run(): Promise<void> {
  console.log();
  if (projectName && typeof projectName === 'string') {
    projectName = projectName.trim();
  }

  if (!projectName) {
    const res = await prompts({
      onState: onPromptState,
      type: 'text',
      name: 'path',
      message: 'What is your project named?',
      initial: 'my-ckb-script',
      validate: (name) => {
        const validation = validateNpmName(path.basename(path.resolve(name)));
        if (validation.valid) {
          return true;
        }
        return 'Invalid project name: ' + (validation.problems?.[0] || 'Unknown validation error');
      },
    });

    if (typeof res.path === 'string') {
      projectName = res.path.trim();
    }
  }

  if (!projectName) {
    console.log(
      '\nPlease specify the project directory:\n' +
      `  ${cyan(program.name())} ${green('<project-directory>')}\n` +
      'For example:\n' +
      `  ${cyan(program.name())} ${green('my-ckb-script')}\n\n` +
      `Run ${cyan(`${program.name()} --help`)} to see all options.`
    );
    process.exit(1);
  }

  const appPath = path.resolve(projectName);
  const appName = path.basename(appPath);

  const validation = validateNpmName(appName);
  if (!validation.valid) {
    console.error(
      `Could not create a project called ${red(
        `"${appName}"`
      )} because of npm naming restrictions:`
    );

    validation.problems?.forEach((p) =>
      console.error(`    ${red(bold('*'))} ${p}`)
    );
    process.exit(1);
  }

  if (fs.pathExistsSync(appPath) && !isFolderEmpty(appPath)) {
    console.error(
      `Could not create a project called ${red(
        `"${appName}"`
      )} because a project with the same name already exists.`
    );
    process.exit(1);
  }

  console.log(bold(`Using ${packageManager}.`));

  const templatePath = path.join(__dirname, `templates`);
  if (!fs.pathExistsSync(templatePath)) {
    console.error(
      `Could not find a template named`
    );
    console.error(`\n 😮‍💨 Project ${projectName} created failed!\n`);
    process.exit(1);
  }

  const originalDirectory = process.cwd();
  const projectPath = path.join(originalDirectory, projectName);

  fs.ensureDirSync(projectPath);
  fs.copySync(templatePath, projectPath);

  updatePackageJson1(projectName)
  updatePackageJson2(projectName)

  console.log(`\n🎉 Project ${projectName} created!\n`);

  if (opts.skipInstall) {
    console.log('Skip install the dependencies, we suggest that you begin by typing:');
    console.log();
    console.log(cyan('  cd'), projectName);
    console.log(`  ${cyan(`${packageManager} install`)}`);
    console.log();
  } else {
    console.log("\nInstalling dependencies:");
    console.log();
    console.log('Installing packages. This might take a couple of minutes.');
    console.log();

    process.chdir(appPath);
    await install(packageManager);
    console.log('Packages installed.');
    console.log();
  }

  console.log(`${green('Success!')} Created ${projectName} at ${projectPath}`);
  console.log();
}

const update = updateCheck(packageJson).catch(() => null);

async function notifyUpdate(): Promise<void> {
  try {
    const updateInfo = await update;
    if (updateInfo?.latest) {
      const global: Record<string, string> = {
        pnpm: 'pnpm add -g',
      };
      const updateMessage = `${global[packageManager]} ${packageJson.name}`;
      console.log(
        yellow(bold(`A new version of \`${packageJson.name}\` is available!`)) +
        '\n' +
        'You can update by running: ' +
        cyan(updateMessage) +
        '\n'
      );
    }
    process.exit(0);
  } catch {
    // ignore error
  }
}

interface ExitReason {
  command?: string;
  message?: string;
}

async function exit(reason: ExitReason): Promise<void> {
  console.log();
  console.log('Aborting installation.');
  if (reason.command) {
    console.log(`  ${cyan(reason.command)} has failed.`);
  } else {
    console.log(
      red('Unexpected error. Please report it as a bug:') + '\n',
      reason
    );
  }
  console.log();
  await notifyUpdate();
  process.exit(1);
}

(async () => {
  try {
    await run();
    await notifyUpdate();
  } catch (error) {
    await exit(error as ExitReason);
  }
})();
