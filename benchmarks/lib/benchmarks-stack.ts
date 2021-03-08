import * as cdk from '@aws-cdk/core';
import * as s3 from '@aws-cdk/aws-s3';
import * as ec2 from '@aws-cdk/aws-ec2';
import * as assets from '@aws-cdk/aws-s3-assets';
import * as iam from '@aws-cdk/aws-iam';
import * as path from 'path';
import * as fs from 'fs';

export class BenchmarksStack extends cdk.Stack {
  constructor(scope: cdk.Construct, id: string, props?: cdk.StackProps) {
    super(scope, id, props);

    const user_name = this.node.tryGetContext('UserName') as string;
    const project_name = this.node.tryGetContext('ProjectName') as string;

    const benchmark_config_json = fs.readFileSync(path.join(__dirname, 'benchmark-config.json'), 'utf8')
    const benchmark_config = JSON.parse(benchmark_config_json)
    const project_config = benchmark_config.projects[project_name];
    const branch_name = project_config.branch_name;
    const local_run_project_sh = project_config.shell_script;

    let region = 'unknown';

    if (props != undefined && props.env != undefined && props.env.region != undefined) {
      region = props.env.region;
    }

    const init_instance_sh = new assets.Asset(this, 'init_instance.sh', {
      path: path.join(__dirname, 'init_instance.sh')
    });

    const show_dashboard_sh = new assets.Asset(this, 'show_instance_dashboard.sh', {
      path: path.join(__dirname, 'show_instance_dashboard.sh')
    });

    const run_project_sh = new assets.Asset(this, local_run_project_sh, {
      path: path.join(__dirname, local_run_project_sh)
    });

    const assetBucket = s3.Bucket.fromBucketName(this, 'AssetBucket', init_instance_sh.s3BucketName)

    const vpc = ec2.Vpc.fromLookup(this, 'VPC', {
      vpcId: 'vpc-305f0856'
    });

    const security_group = new ec2.SecurityGroup(this, 'SecurityGroup', {
      vpc: vpc,
    });

    security_group.addIngressRule(
      ec2.Peer.anyIpv4(),
      ec2.Port.tcp(22),
      'SSH'
    );

    const canary_role = iam.Role.fromRoleArn(this, 'S3CanaryInstanceRole', 'arn:aws:iam::123124136734:role/S3CanaryEC2Role');

    for (let instance_config_name in benchmark_config.instances) {
      const instance_config = benchmark_config.instances[instance_config_name]
      const instance_user_data = ec2.UserData.forLinux();

      const init_instance_sh_path = instance_user_data.addS3DownloadCommand({
        bucket: assetBucket,
        bucketKey: init_instance_sh.s3ObjectKey
      });

      const show_dashboard_sh_path = instance_user_data.addS3DownloadCommand({
        bucket: assetBucket,
        bucketKey: show_dashboard_sh.s3ObjectKey
      });

      const run_project_sh_path = instance_user_data.addS3DownloadCommand({
        bucket: assetBucket,
        bucketKey: run_project_sh.s3ObjectKey
      })

      const init_instance_arguments = user_name + ' ' +
        show_dashboard_sh_path + ' ' +
        project_name + ' ' +
        branch_name + ' ' +
        instance_config.throughput_gbps + ' ' +
        run_project_sh_path + ' ' +
        instance_config_name + ' ' +
        region + ' ';

      instance_user_data.addExecuteFileCommand({
        filePath: init_instance_sh_path,
        arguments: init_instance_arguments
      });

      const ec2instance = new ec2.Instance(this, 'S3BenchmarkClient_' + instance_config_name, {
        instanceType: new ec2.InstanceType(instance_config_name),
        vpc: vpc,
        machineImage: ec2.MachineImage.latestAmazonLinux({ generation: ec2.AmazonLinuxGeneration.AMAZON_LINUX_2 }),
        userData: instance_user_data,
        role: canary_role,
        keyName: 'aws-common-runtime-keys',
        securityGroup: security_group
      });
    }
  }
}
